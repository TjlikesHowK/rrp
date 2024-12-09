#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include "ipc.h"
#include "banking.h"
void create_pipe_topology(int8_t num_processes, FILE *pipelog);
void close_unused_pipes(int8_t num_processes, local_id id);
void close_used_pipes(int8_t num_processes, local_id id);
void process_send_multicast(local_id id, int16_t type);
void process_send(local_id from, local_id to, int16_t type, TransferOrder *order, BalanceHistory *history);
void process_recieve_all(int8_t num_processes, local_id id, int16_t type);
void process_recieve_any(local_id id);
void process_load(local_id id);
