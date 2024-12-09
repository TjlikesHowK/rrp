#include "business_load.h"
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include "banking.h"
#include "pa2345.h"
#include "transmission_handler.h"
extern FILE *eventlog;
extern int num_processes;
BalanceState balstate;
BalanceHistory balhist;
AllHistory *allhist = NULL;
timestamp_t lamport_time = 0;
timestamp_t transaction_time_by_dst[11] = {0};
balance_t pending_balance_by_dst[11] = {0};
int stop_flag = 0;
int ack_flag = 1;
timestamp_t get_lamport_time()
{
    return lamport_time;
}
balance_t get_balance()
{
    return balstate.s_balance;
}
void create_all_history(int num_processes)
{
    allhist = malloc(sizeof(uint8_t)+num_processes*sizeof(BalanceHistory));
    allhist->s_history_len = num_processes;
}
void reset_balance_history(local_id id)
{
    balhist.s_id = id;
    balhist.s_history_len = 1;
    balhist.s_history[0] = balstate;
}
void reset_balance_state(balance_t bal)
{
    balstate.s_balance = bal;
    balstate.s_time = get_lamport_time();
    balstate.s_balance_pending_in = 0;
}
void send_balance_history()
{
    process_send(balhist.s_id, PARENT_ID, BALANCE_HISTORY, NULL, &balhist);
}
void process_msg_ack(Message *msg, local_id src, local_id dst)
{
    ack_flag = 1;
    if (dst != 0)
    {
        timestamp_t end_time = msg->s_header.s_local_time;
        timestamp_t iter = balstate.s_time + 1;
        while (iter < end_time)
        {
            balstate.s_time = iter;
            balhist.s_history[balhist.s_history_len] = balstate;
            balhist.s_history_len++;
            iter++;
        }
        balstate.s_balance_pending_in -= pending_balance_by_dst[src];
        pending_balance_by_dst[src] = 0;
        balstate.s_time = end_time;
        balhist.s_history[balhist.s_history_len] = balstate;
        balhist.s_history_len++;
    }
}
void process_balance_history(Message *msg)
{
    memcpy(&balhist, msg->s_payload, msg->s_header.s_payload_len);
    allhist->s_history[balhist.s_id - 1] = balhist;
}
void add_transaction(balance_t ammount, local_id dst) {
    timestamp_t current_time = get_lamport_time();
    
    if (ammount == 0) {
        current_time = 254;
    }
    timestamp_t iter_time = balstate.s_time + 1;
    for (; iter_time < current_time; iter_time++) {
        balstate.s_time = iter_time;
        balhist.s_history[balhist.s_history_len++] = balstate;
    }
    balstate.s_balance += ammount;
    if (ammount < 0) {
        transaction_time_by_dst[dst] = current_time;
        pending_balance_by_dst[dst] += -ammount;
        balstate.s_balance_pending_in += -ammount;
    }
    balstate.s_time = current_time;
    balhist.s_history[balhist.s_history_len++] = balstate;
}
void process_msg_transfer(Message *msg, local_id id)
{
    TransferOrder *order = malloc(sizeof(TransferOrder));
    order->s_src = ((TransferOrder *)(msg->s_payload))->s_src;
    order->s_dst = ((TransferOrder *)(msg->s_payload))->s_dst;
    order->s_amount = ((TransferOrder *)(msg->s_payload))->s_amount;
    if (id == order->s_src)
    {
        if (stop_flag == 0)
        {
            add_transaction(-order->s_amount, order->s_dst);
            process_send(id, order->s_dst, TRANSFER, order, NULL);
        }
    }
    if (id == order->s_dst)
    {
        process_send(id, order->s_src, ACK, NULL, NULL);
        add_transaction(order->s_amount, order->s_dst);
        process_send(id, PARENT_ID, ACK, NULL, NULL);
    }
    free(order);
}
void process_msg_stop(Message *msg)
{
    stop_flag = 1;
}
void transfer(void * parent_data, local_id src, local_id dst,
              balance_t amount)
{
    TransferOrder *order = malloc(sizeof(TransferOrder));
    order->s_src = src;
    order->s_dst = dst;
    order->s_amount = amount;
    process_send(PARENT_ID, src, TRANSFER, order, NULL);
    ack_flag = 0;
    while(!ack_flag)
    {
        process_recieve_any(PARENT_ID);
    }
    free(order);
}
void load(local_id id)
{
    while (!stop_flag)
    {
        process_recieve_any(id);
    }
    add_transaction(0, id);
}
