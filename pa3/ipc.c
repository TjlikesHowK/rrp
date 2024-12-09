#include "ipc.h"
#include <unistd.h>
local_id any_dst = -1;
int send(void *self, local_id dst, const Message *msg) {
    if (!self || !msg) {
        return -1;
    }
    int socket_fd = ((int *)self)[dst];
    size_t message_size = sizeof(MessageHeader) + msg->s_header.s_payload_len;
    if (write(socket_fd, msg, message_size) == -1) {
        return -1;
    }
    return 0;
}
int send_multicast(void *self, const Message *msg) {
    if (!self || !msg) {
        return -1;
    }
    local_id i = 0;
    int *ids = (int *)self;
    while (ids[i] != -2) {
        if (ids[i] != -1 && send(self, i, msg) != 0) {
            return -1;
        }
        i++;
    }
    
    return 0;
}
int receive(void *self, local_id from, Message *msg) {
    if (!self || !msg) {
        return -1;
    }
    MessageHeader msg_hdr;
    int self_id = ((int *)self)[from];
    if (read(self_id, &msg_hdr, sizeof(MessageHeader)) == sizeof(MessageHeader)) {
        msg->s_header = msg_hdr;
        if (read(self_id, msg->s_payload, msg_hdr.s_payload_len) == msg_hdr.s_payload_len) {
            return 0;
        }
        return -1;
    }
    
    return 1;
}
int receive_any(void *self, Message *msg)
{
    if (!self || !msg) {
        return -1;
    }
    for (local_id i = 0; ((int *)self)[i] != -2; i++)
    {
        if (((int *)self)[i] != -1)
        {
            any_dst = i;
            int res = receive(self, i, msg);
            if (res != 1) 
                return res;
        }
    }
    return 1;
}
