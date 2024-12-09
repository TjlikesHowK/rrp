#include "handle_message.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "banking.h"
#include "pa2345.h"
#include "business_load.h"
extern FILE *eventlog;
extern timestamp_t lamport_time;
int started[11] = {0};
int done[11] = {0};
int balance[11] = {0};
int8_t started_num = 0;
int8_t done_num = 0;
int8_t balance_num = 0;
int started_len = 0;
int done_len = 0;
void process_msg_started(Message *msg) {
	int id, pid, ppid, time, bal;
	msg->s_payload[msg->s_header.s_payload_len] = '\0';
	sscanf(msg->s_payload, log_started_fmt, &time, &id, &pid, &ppid, &bal);
	if (!started[id]) ++started_num;
	++started[id];
}
void process_msg_done(Message *msg) {
	int id, time, bal;
	msg->s_payload[msg->s_header.s_payload_len] = '\0';
	sscanf(msg->s_payload, log_done_fmt, &time, &id, &bal);
	if (!done[id]) ++done_num;
	++done[id];
}
void process_msg(Message *msg, local_id id, local_id src)
{
	lamport_time++;
	while (lamport_time <= msg->s_header.s_local_time) {
		lamport_time++;
	}
	int16_t tmp = msg->s_header.s_type;
	if (tmp == STARTED) {
		process_msg_started(msg);
	} else if (tmp == DONE) {
		process_msg_done(msg);
	} else if (tmp == STOP) {
		process_msg_stop(msg);
	} else if (tmp == TRANSFER) {
		process_msg_transfer(msg, id);
	} else if (tmp == ACK) {
		process_msg_ack(msg, src, id);
	} else if (tmp == BALANCE_HISTORY) {
		process_balance_history(msg);
	} else {
		fclose(eventlog);
		free(msg);
		perror("Unknown msg type");
		exit(EXIT_FAILURE);
	}
}
int payload_size(int16_t type, BalanceHistory *history) {
	if (type == STARTED) {
     		return started_len;
     	} else if (type == DONE) {
		return done_len;
     	} else if (type == STOP) {
     		return 0;
     	} else if (type == TRANSFER) {
     		return sizeof(TransferOrder);
     	} else if (type == ACK) {
     		return 0;
     	} else if (type == BALANCE_HISTORY) {
     		return sizeof(local_id) + sizeof(uint8_t) + history->s_history_len*sizeof(BalanceState);
     	} else {
     		return MAX_PAYLOAD_LEN;
     	}
	fclose(eventlog);
	perror("err");
	exit(EXIT_FAILURE);
}
Message *create_msg(int16_t type, char *payload, BalanceHistory *history) {
	Message *msg = malloc(sizeof(MessageHeader)+payload_size(type, history));;
	
	msg->s_header.s_magic = MESSAGE_MAGIC;
	msg->s_header.s_payload_len = payload_size(type, history);
	msg->s_header.s_type = type;
	if (type >= 0)
	{
		lamport_time++;
	}
	msg->s_header.s_local_time = get_lamport_time();
	for (int i = 0; i < payload_size(type, history); i++) {
		msg->s_payload[i] = payload[i];
	}
	
	return msg;
}
void create_typed_payload(int16_t type, local_id id, TransferOrder *order, BalanceHistory *history, char *payload, uint16_t payload_len) {
	if (type == STARTED) {
	        sprintf(payload, log_started_fmt, get_lamport_time()+1, id, getpid(), getppid(), get_balance());
     	} else if (type == DONE) {
	        sprintf(payload, log_done_fmt, get_lamport_time()+1, id, get_balance());
     	} else if (type == TRANSFER) {
		if (order == NULL) {
			fclose(eventlog);
	            	perror("create_payload");
	            	exit(EXIT_FAILURE);
		}
		memcpy(payload, order, sizeof(TransferOrder));
     	} else if (type == BALANCE_HISTORY) {
     		if (history == NULL) {
			fclose(eventlog);
	            	perror("create_payload");
	            	exit(EXIT_FAILURE);
		}
		memcpy(payload, history, payload_len);
     	}
}
char *create_payload(int16_t type,
                     local_id id,
                     TransferOrder *order,
                     BalanceHistory *history)
{
    uint16_t payload_len;
	payload_len = payload_size(type, history);
	char *payload = malloc(payload_len);
	if (payload == NULL && payload_len != 0)
	{
	    fclose(eventlog);
	    perror("create_payload");
	    exit(EXIT_FAILURE);
	}
    create_typed_payload(type, id, order, history, payload, payload_len);
	return payload;
}
void count_sent_num(local_id id, int16_t type) {
	if (type == STARTED) {
     		started[id]++;
		started_num++;
     	} else if (type == DONE) {
		done[id]++;
		done_num++;
     	} else if (type == BALANCE_HISTORY) {
     		balance[id]++;
		balance_num++;
     	}
}
int8_t *get_rcvd_num(int16_t type) {
     	if (type == STARTED) {
     		return &started_num;
     	} else if (type == DONE) {
     		return &done_num;
     	} else if (type == BALANCE_HISTORY) {
     		return &balance_num;
     	} else {
     		fclose(eventlog);
		perror("Unknown msg type");
		exit(EXIT_FAILURE);
     	}
     	
	fclose(eventlog);
	perror("err");
	exit(EXIT_FAILURE);
}
int *get_rcvd(int16_t type) {
	if (type == STARTED) {
		return started;
	} else if (type == DONE) {
		return done;
	} else if (BALANCE_HISTORY) {
		return balance;
	} else {
		fclose(eventlog);
		perror("Unknown msg type");
		exit(EXIT_FAILURE);
	}
	
	perror("err");
	exit(EXIT_FAILURE);
}
