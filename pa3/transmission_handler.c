#include "transmission_handler.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "business_load.h"
#include "handle_message.h"
extern FILE *eventlog;
extern local_id any_dst;
char free_payload[MAX_PAYLOAD_LEN];
int pipefds_to_write[12][12];
int pipefds_to_read[12][12];
#define INVALID_FD -1
#define TERMINATOR_FD -2
void create_pipe_topology(int8_t num_processes, FILE *pipelog)
{
    int pipefd[2];
    
    if (num_processes <= 0 || pipelog == NULL) {
        fprintf(stderr, "Неверные параметры.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i <= num_processes; i++) {
        for (int j = 0; j <= num_processes; j++) {
            if (i == j) {
                pipefds_to_read[j][i] = INVALID_FD;
                pipefds_to_write[i][j] = INVALID_FD;
                continue;
            }
            if (pipe2(pipefd, O_NONBLOCK) == -1) {
                perror("Ошибка создания трубы");
                fclose(pipelog);
                exit(EXIT_FAILURE);
            }
            fprintf(pipelog, "%i %i\n", pipefd[0], pipefd[1]);
            pipefds_to_read[j][i] = pipefd[0];
            pipefds_to_write[i][j] = pipefd[1];
        }
        
        pipefds_to_read[i][num_processes + 1] = TERMINATOR_FD;
        pipefds_to_write[i][num_processes + 1] = TERMINATOR_FD;
        pipefds_to_read[num_processes + 1][i] = TERMINATOR_FD;
        pipefds_to_write[num_processes + 1][i] = TERMINATOR_FD;
    }
    
    if (fclose(pipelog) != 0) {
        perror("Ошибка закрытия лог-файла");
        exit(EXIT_FAILURE);
    }
}
void close_pipe(int pipe_fd, const char *func_name) {
    if (pipe_fd != -1) {
        if (close(pipe_fd) != 0) {
            perror(func_name);
            exit(EXIT_FAILURE);
        }
    }
}
void close_unused_pipes(int8_t num_processes, local_id id) {
    local_id i, j;
    for (i = 0; i <= num_processes; i++) {
        if (i == id) {
            continue;
        }
        for (j = 0; j <= num_processes; j++) {
            close_pipe(pipefds_to_read[i][j], "close_unused_pipes");
            close_pipe(pipefds_to_write[i][j], "close_unused_pipes");
        }
    }
}
void close_used_pipes(int8_t num_processes, local_id id) {
    for (local_id j = 0; j <= num_processes; ++j) {
        if (pipefds_to_read[id][j] != -1 && close(pipefds_to_read[id][j]) != 0) {
            fclose(eventlog);
            perror("close_used_pipes");
            exit(EXIT_FAILURE);
        }
        pipefds_to_read[id][j] = -1;
        if (pipefds_to_write[id][j] != -1 && close(pipefds_to_write[id][j]) != 0) {
            fclose(eventlog);
            perror("close_used_pipes");
            exit(EXIT_FAILURE);
        }
        pipefds_to_write[id][j] = -1;
    }
}
void process_send_multicast(local_id id, int16_t type) {
    char *payload = create_payload(type, id, NULL, NULL);
    Message *msg = create_msg(type, payload, NULL);
    free(payload);
    if (send_multicast(pipefds_to_write[id], msg) != 0) {
        fclose(eventlog);
        free(msg);
        perror("send_multicast");
        exit(EXIT_FAILURE);
    }
    count_sent_num(id, type);
    free(msg);
}
void process_send(local_id from, local_id to, int16_t type, TransferOrder *order, BalanceHistory *history) {
    char *payload = create_payload(type, from, order, history);
    Message *msg = create_msg(type, payload, history);
    free(payload);
    if (send(pipefds_to_write[from], to, msg) != 0) {
        fclose(eventlog);
        free(msg);
        perror("send");
        exit(EXIT_FAILURE);
    }
    count_sent_num(from, type);
    free(msg);
}
void process_recieve_all(int8_t num_processes, local_id id, int16_t type) {
    int8_t *received_count = get_rcvd_num(type);
    int *received_flags = get_rcvd(type);
    int watchdog = 0;
    const int max_watchdog = 1000;
    while (*received_count < num_processes && watchdog < max_watchdog) {
        for (local_id i = 1; i <= num_processes; i++) {
            if (received_flags[i] != 0) {
                continue;
            }
            Message *msg = create_msg(-1, free_payload, NULL);
            int res = receive(pipefds_to_read[id], i, msg);
            if (res == 0) {
                process_msg(msg, id, i);
            } else if (res == -1) {
                fclose(eventlog);
                free(msg);
                perror("receive_any");
                exit(EXIT_FAILURE);
            }
            free(msg);
        }
        watchdog++;
    }
}
void process_recieve_any(local_id id) {
    Message *message = (Message *)malloc(sizeof(Message));
    if (!message) {
        perror("Ошибка при выделении памяти для сообщения");
        exit(EXIT_FAILURE);
    }
    int receive_result = receive_any(pipefds_to_read[id], message);
    if (receive_result == -1) {
        perror("Ошибка при получении сообщения через receive_any");
        if (eventlog) fclose(eventlog);
        free(message);
        exit(EXIT_FAILURE);
    }
    if (receive_result == 0) {
        process_msg(message, id, any_dst);
    }
    if (message) {
        free(message);
    }
}
void process_load(local_id id)
{
    load(id);
}
