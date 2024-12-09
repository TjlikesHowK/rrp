#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include "banking.h"
#include "common.h"
#include "ipc.h"
#include "business_load.h"
#include "pa2345.h"
#include "transmission_handler.h"
extern int started_len;
extern int done_len;
extern char free_payload[MAX_PAYLOAD_LEN];
balance_t cash[11];
extern AllHistory *allhist;
extern timestamp_t lamport_time;
FILE *eventlog;
void child(int total_procs, local_id proc_id) {
	reset_balance_state(cash[proc_id]);
	reset_balance_history(proc_id);
	lamport_time = proc_id - 1;
	process_send_multicast(proc_id, STARTED);
	process_recieve_all(total_procs, proc_id, STARTED);
	process_load(proc_id);
	process_send_multicast(proc_id, DONE);
	process_recieve_all(total_procs, proc_id, DONE);
	send_balance_history();
}
void spawn_processes(int total_procs) {
	local_id idx;
	for (idx = 0; idx < total_procs; idx++) {
		int16_t pid = fork();
		if (pid == -1) {
			perror("fork");
			fclose(eventlog);
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {
		    close_unused_pipes(total_procs, idx + 1);
		    child(total_procs, idx + 1);
			close_used_pipes(total_procs, idx + 1);
			fclose(eventlog);
			exit(EXIT_SUCCESS);
		}
	}
}
void monitor_processes(int total_procs) {
	int16_t pid;
	while (total_procs > 0) {
		pid = waitpid(-1, NULL, WNOHANG);
		if (pid == -1) {
			fputs("wait error", stderr);
			fclose(eventlog);
			exit(EXIT_FAILURE);
		} else if (pid > 0) {
			total_procs--;
		}
	}
}
void parent_process(int procs_count, FILE *pipe_log) {
	create_all_history(procs_count);
	create_pipe_topology(procs_count, pipe_log);
	spawn_processes(procs_count);
	close_unused_pipes(procs_count, PARENT_ID);
	process_recieve_all(procs_count, PARENT_ID, STARTED);
	bank_robbery(NULL, procs_count);
	process_send_multicast(PARENT_ID, STOP);
	process_recieve_all(procs_count, PARENT_ID, DONE);
	process_recieve_all(procs_count, PARENT_ID, BALANCE_HISTORY);
	print_history(allhist);
	monitor_processes(procs_count);
	close_used_pipes(procs_count, PARENT_ID);
}
void display_error(char *argv[]) {
	fprintf(stderr, "Usage Error: %s -p N S1 S2 ... SN\n", argv[0]);
	exit(EXIT_FAILURE);
}
int main(int argc, char *argv[]) {
	int procs_count = 0;
	const char *opts = "p";
	char opt;
	FILE *pipe_log;
	if (argc < 3) {
		display_error(argv);
	}
	while ((opt = getopt(argc, argv, opts)) != -1) {
		switch (opt) {
			case 'p':
				procs_count = atoi(argv[optind]);
				if (procs_count < 0) {
					display_error(argv);
				}
				break;
		}
	}
	for (local_id idx = 0; idx < procs_count; idx++) {
		cash[idx + 1] = atoi(argv[idx + 3]);
	}
	pipe_log = fopen(pipes_log, "w");
	if (pipe_log == NULL) {
		perror("Pipe log file opening error");
		exit(EXIT_FAILURE);
	}
	eventlog = fopen(events_log, "a");
	if (eventlog == NULL) {
		perror("Event log file opening error");
		fclose(pipe_log);
		exit(EXIT_FAILURE);
	}
	sprintf(free_payload, log_started_fmt, 35000, PARENT_ID, getpid(), getppid(), 99);
	started_len = strlen(free_payload);
	sprintf(free_payload, log_done_fmt, 35000, PARENT_ID, 99);
	done_len = strlen(free_payload);
	parent_process(procs_count, pipe_log);
	fclose(eventlog);
	return EXIT_SUCCESS;
}
