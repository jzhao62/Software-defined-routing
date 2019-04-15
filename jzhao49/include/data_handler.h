//
// Created by bilin on 11/29/18.
//

#ifndef BILINSHI_DATA_HANDLER_H
#define BILINSHI_DATA_HANDLER_H

#include <stdint.h>

struct DataConn{
    uint32_t router_ip;
    int socket;
};
bool isData(int sock_index);

int create_data_sock(uint16_t data_port);

int new_data_conn(int sock_index);

int get_data_sock(uint32_t remote_ip);

int create_data_conn(uint32_t remote_ip, uint16_t remote_port);

void remove_data_conn(int sock_index);

bool data_recv_hook(int sock_index);

void close_all_data_sock();

#endif //BILINSHI_DATA_HANDLER_H
