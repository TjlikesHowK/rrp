#include <stdint.h>
#include "ipc.h"
#include "banking.h"

void process_msg(Message *msg, local_id dst, local_id src);
Message *create_msg(int16_t type, char *payload, BalanceHistory *history);
char *create_payload(int16_t type, local_id id, TransferOrder *order, BalanceHistory *history);
void count_sent_num(local_id id, int16_t type);
int8_t *get_rcvd_num(int16_t type);
int *get_rcvd(int16_t type);
