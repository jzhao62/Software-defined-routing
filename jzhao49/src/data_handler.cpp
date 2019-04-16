//
// Created by bilin on 11/29/18.
//

#include "../include/data_handler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <algorithm>
#include "../include/global.h"
#include "control_header_lib.h"
#include "../include/connection_manager.h"
#include "../include/control_handler.h"
#include <iostream>
#include <network_utils.h>

using namespace std;

vector<DataConn> data_socket_list;
int data_socket;

int create_data_conn(uint32_t remote_ip, uint16_t remote_port) {
    int sock;
    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) ERROR("socket() failed");

    bzero(&data_addr, sizeof(data_addr));

    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(remote_ip);
    data_addr.sin_port = htons(remote_port);

    if (connect(sock, (struct sockaddr *) &data_addr, sizeof(data_addr)) < 0) ERROR("connect() failed");
    return sock;
}

void remove_data_conn(int sock_index) {
    for (int i = 0; i < data_socket_list.size(); i++) {
        if (data_socket_list[i].socket == sock_index) {
            data_socket_list.erase(data_socket_list.begin() + i);
            break;
        }
    }

    close(sock_index);
}

