#include <stdint.h>
#include "banking.h"
#include "ipc.h"
void load(local_id id);
void process_msg_transfer(Message *msg, local_id id);
void process_msg_stop(Message *msg);
void process_msg_ack(Message *msg, local_id src, local_id dst);
void process_balance_history(Message *msg);
void create_all_history(int num_processes);
void reset_balance_history(local_id id);
void reset_balance_state(balance_t bal);
void send_balance_history();
balance_t get_balance();
