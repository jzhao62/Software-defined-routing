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
#include "../include/header.h"
#include "../include/utils.h"
#include "../include/connection_manager.h"
#include "../include/control_handler.h"
#include <iostream>

using namespace std;

vector<DataConn> data_socket_list;
int data_socket;


bool isData(int sock_index) {
    for (int i = 0; i < data_socket_list.size(); i++) {
        if (data_socket_list[i].socket == sock_index) {
            return true;
        }
    }
    return false;
}

int create_data_sock(uint16_t data_port) {
    int sock;
    struct sockaddr_in data_addr;
    socklen_t addrlen = sizeof(data_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) ERROR("socket() failed");

    /* Make socket re-usable */
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(opt)) < 0) ERROR("setsockopt() failed");

    bzero(&data_addr, sizeof(data_addr));

    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    data_addr.sin_port = htons(data_port);

    if (bind(sock, (struct sockaddr *) &data_addr, sizeof(data_addr)) < 0) ERROR("bind() failed");

    if (listen(sock, 5) < 0) ERROR("listen() failed");

    return sock;
}

int new_data_conn(int sock_index) {
    int fdaccept, caddr_len;
    struct sockaddr_in remote_data_addr;

    caddr_len = sizeof(remote_data_addr);
    fdaccept = accept(sock_index, (struct sockaddr *) &remote_data_addr, (socklen_t *) &caddr_len);
    if (fdaccept < 0) ERROR("accept() failed");

    /* Insert into list of active control connections */
    struct DataConn new_conn;
    new_conn.socket = fdaccept;
    new_conn.router_ip = ntohl(remote_data_addr.sin_addr.s_addr);
    data_socket_list.push_back(new_conn);

    FD_SET(fdaccept, &master_list);
    if (fdaccept > head_fd) head_fd = fdaccept;

    return fdaccept;
}

int get_data_sock(uint32_t remote_ip) {
    int expected_socket;
    bool has_connected = false;
    for (int i = 0; i < data_socket_list.size(); i++) {
        if (data_socket_list[i].router_ip == remote_ip) {
            expected_socket = data_socket_list[i].socket;
            has_connected = true;
            break;
        }
    }
    if (!has_connected) {
        uint16_t remote_port;

        for (int i = 0; i < table.size(); i++) {
            if (table[i].dest_ip == remote_ip) {
                remote_port = table[i].dest_data_port;
                break;
            }
        }
        expected_socket = create_data_conn(remote_ip, remote_port);
    }
    return expected_socket;
}

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

bool data_recv_hook(int sock_index) {
    char *data_header, *data_payload;
    uint32_t dest_ip;
    uint8_t transfer_id;
    uint8_t ttl;
    uint16_t seq_num;
    uint32_t fin;

    data_header = (char *) malloc(sizeof(char) * DATA_HEADER_SIZE);
    bzero(data_header, DATA_HEADER_SIZE);

    if (recvALL(sock_index, data_header, DATA_HEADER_SIZE) < 0) {
        remove_data_conn(sock_index);
        free(data_header);
        return false;
    }
    memcpy(&dest_ip, data_header, sizeof(dest_ip));
    dest_ip = ntohl(dest_ip);
    memcpy(&transfer_id, data_header + 4, sizeof(transfer_id));
    memcpy(&ttl, data_header + 5, sizeof(ttl));
    memcpy(&seq_num, data_header + 6, sizeof(seq_num));
    seq_num = ntohs(seq_num);
    memcpy(&fin, data_header + 8, sizeof(fin));

    free(data_header);

    data_payload = (char *) malloc(sizeof(char) * DATA_PAYLOAD);
    bzero(data_payload, DATA_PAYLOAD);

    if (recvALL(sock_index, data_payload, DATA_PAYLOAD) < 0) {
        remove_data_conn(sock_index);
        free(data_payload);
        return false;
    }

    char *packet;
    packet = (char *) malloc(sizeof(char) * (DATA_HEADER_SIZE + DATA_PAYLOAD));
    bzero(packet, DATA_HEADER_SIZE + DATA_PAYLOAD);
    memcpy(packet, data_header, DATA_HEADER_SIZE);
    memcpy(packet + DATA_HEADER_SIZE, data_payload, DATA_PAYLOAD);

//    bzero(penultimate_packet, DATA_PAYLOAD + 12);
//    memcpy(penultimate_packet, last_packet, DATA_PAYLOAD + 12);
//    bzero(last_packet, DATA_PAYLOAD + 12);
//    memcpy(last_packet, packet, DATA_PAYLOAD + 12);

    ttl--;
    if (dest_ip == my_ip) {
        memcpy(file_buffer + datagram_count * DATA_PAYLOAD, data_payload, DATA_PAYLOAD);
        datagram_count++;
        if (fin == LAST_DATA_PACKET) {
            /* complete buffer, write to file */
            char file_name[80];
            strcpy(file_name, "file-<");
            char trans_id[10];
            sprintf(trans_id, "%d", transfer_id);
            strcpy(file_name, trans_id);
            strcpy(file_name, ">");
            cout << "file name: " << file_name << endl;
            FILE *fw = fopen(file_name, "w");
            char buffer[DATA_PAYLOAD];
            for (int i = 0; i < datagram_count; i++) {
                memcpy(buffer, file_buffer + (i * DATA_PAYLOAD), DATA_PAYLOAD);
                if (fwrite(buffer, sizeof(char), DATA_PAYLOAD, fw) < DATA_PAYLOAD) {
                    printf("File:\t%s Write Failed\n", file_name);
                    break;
                }
                bzero(buffer, DATA_PAYLOAD);
            }
            bzero(file_buffer, 10000 * DATA_PAYLOAD);
            fclose(fw);
            datagram_count = 0;
        }
        bzero(penultimate_packet, DATA_PAYLOAD + 12);
        memcpy(penultimate_packet, last_packet, DATA_PAYLOAD + 12);
        bzero(last_packet, DATA_PAYLOAD + 12);
        memcpy(last_packet, packet, DATA_PAYLOAD + 12);
        return true;
    } else {
        // store ttl bind with transfer id first
        if (ttl == 0) {
            cout << "ttl timeout, drop it" << endl;
            free(data_payload);
            return true;
        } else {
            bool find_transfer_id = false;
            for (int i = 0; i < transfer_files.size(); i++) {
                if (transfer_files[i].transfer_id == transfer_id) {
                    transfer_files[i].sequence.push_back(seq_num);
                    find_transfer_id = true;
                    break;
                }
            }
            if (!find_transfer_id) {
                struct Transfer_File transfer_file;
                transfer_file.transfer_id = transfer_id;
                transfer_file.ttl = ttl;
                transfer_file.sequence.push_back(seq_num);
                transfer_files.push_back(transfer_file);
            }
            bzero(penultimate_packet, DATA_PAYLOAD + 12);
            memcpy(penultimate_packet, last_packet, DATA_PAYLOAD + 12);
            bzero(last_packet, DATA_PAYLOAD + 12);
            memcpy(last_packet, packet, DATA_PAYLOAD + 12);
        }
    }

    char *next_data_header = create_data_header(dest_ip, transfer_id, ttl, seq_num, fin);
    char *next_packet = (char *) malloc(DATA_HEADER_SIZE + DATA_PAYLOAD);
    memcpy(next_packet, next_data_header, DATA_HEADER_SIZE);
    memcpy(next_packet + DATA_HEADER_SIZE, data_payload, DATA_PAYLOAD);

    // need to decide which router to send
    // if not connected yet, create new socket and add to master_list
    int expected_socket = -1;
    uint16_t next_hop;
    uint32_t next_hop_ip;
    for (int i = 0; i < table.size(); i++) {
        if (table[i].dest_ip == dest_ip) {
            next_hop = table[i].next_hop_id;
            break;
        }
    }
    for (int j = 0; j < table.size(); j++) {
        if (table[j].dest_id == next_hop) {
            next_hop_ip = table[j].dest_ip;
            break;
        }
    }
    for (int k = 0; k < data_socket_list.size(); k++) {
        if (data_socket_list[k].router_ip == next_hop_ip) {
            expected_socket = data_socket_list[k].socket;
            break;
        }
    }
    if (expected_socket < 0) {
        cout << "didn't find next hop, it's not possible" << endl;
    }
    sendALL(expected_socket, next_packet, DATA_HEADER_SIZE + DATA_PAYLOAD);


    return true;
}

void close_all_data_sock() {
    for (int i = 0; i < data_socket_list.size(); i++) {
        close(data_socket_list[i].socket);
    }
}