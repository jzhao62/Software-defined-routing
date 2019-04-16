//
// Created by bilin on 11/29/18.
//

#include "../include/control_handler.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <algorithm>
#include "../include/global.h"
#include "control_header_lib.h"
#include "../include/connection_manager.h"
#include "../include/data_handler.h"
#include "network_utils.h"
#include <iostream>
#include <map>
using namespace std;

vector<int> control_conn_list;

map<uint16_t , Routing > routing_table;

Routing self;



int router_socket;



int create_control_sock(uint16_t CONTROL_PORT) {
    int sock;
    struct sockaddr_in control_addr;
    socklen_t addrlen = sizeof(control_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) ERROR("socket() failed");

    /* Make socket re-usable */
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(opt)) < 0) ERROR("setsockopt() failed");

    bzero(&control_addr, sizeof(control_addr));

    control_addr.sin_family = AF_INET;
    control_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    control_addr.sin_port = htons(CONTROL_PORT);

    if (bind(sock, (struct sockaddr *) &control_addr, sizeof(control_addr)) < 0) ERROR("bind() failed");

    if (listen(sock, 5) < 0) ERROR("listen() failed");


    return sock;
}

int new_control_conn(int sock_index) {
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *) &remote_controller_addr, (socklen_t *) &caddr_len);
    if (fdaccept < 0) ERROR("accept() failed");

    /* Insert into list of active control connections */
    control_conn_list.push_back(fdaccept);

    FD_SET(fdaccept, &master_list);
    if (fdaccept > head_fd) head_fd = fdaccept;

    return fdaccept;
}

void remove_control_conn(int sock_index) {
    for (int i = 0; i < control_conn_list.size(); i++) {
        if (control_conn_list[i] == sock_index) {
            control_conn_list.erase(control_conn_list.begin() + i);
            break;
        }
    }

    close(sock_index);
}

bool isControl(int sock_index) {
    vector<int>::iterator it = find(control_conn_list.begin(), control_conn_list.end(), sock_index);
    return it != control_conn_list.end();
}


bool control_recv_hook(int sock_index) {
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char) * CONTROL_HEADER_SIZE);
    bzero(cntrl_header, CONTROL_HEADER_SIZE);

    if (recvALL(sock_index, cntrl_header, CONTROL_HEADER_SIZE) < 0) {
        remove_control_conn(sock_index);
        free(cntrl_header);
        return false;
    }

    /* Get control code and payload length from the header */

    memcpy(&control_code, cntrl_header + CONTROL_CODE_OFFSET, sizeof(control_code));
    memcpy(&payload_len, cntrl_header + PAYLOAD_LEN_OFFSET, sizeof(payload_len));
    payload_len = ntohs(payload_len);


    free(cntrl_header);

    /* Get control payload */
    if (payload_len != 0) { // in this case, must greater than 0
        cntrl_payload = (char *) malloc(sizeof(char) * payload_len);
        bzero(cntrl_payload, payload_len);

        if (recvALL(sock_index, cntrl_payload, payload_len) < 0) {
            remove_control_conn(sock_index);
            free(cntrl_payload);
            return false;
        }
    }

    cout << endl;
    cout << "receive controller message: " <<  (int)control_code << endl;
    cout << endl;

    /* Triage on control_code */
    switch (control_code) {
        case AUTHOR:
            author(sock_index);
            break;
        case INIT:
            initialize(sock_index, cntrl_payload);


            break;
        case ROUTING_TABLE:
            break;
        case UPDATE:
            break;
        case CRASH:
            break;
        case SENDFILE:
            break;
        case SENDFILE_STATS:
            break;
        case LAST_DATA_PACKET:
            break;
        case PENULTIMATE_DATA_PACKET:
            break;

        default:
            break;
    }

    if (payload_len != 0)free(cntrl_payload);

    return true;
}











void author(int sock_index) {
    uint16_t payload_len, response_len;
    char *ctrl_response_header, *ctrl_response_payload, *ctrl_response;

    payload_len = sizeof(AUTHOR_STATEMENT) - 1; // Discount the NULL character
    ctrl_response_payload = (char *) malloc(payload_len);
    memcpy(ctrl_response_payload, AUTHOR_STATEMENT, payload_len);
    cout << AUTHOR_STATEMENT << endl;

    ctrl_response_header = create_response_header(sock_index, 0, 0, payload_len);

    response_len = CONTROL_HEADER_SIZE + payload_len;
    ctrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, payload_len);

    sendALL(sock_index, ctrl_response, response_len);
}


void initialize(int sock_index, char* payload){


    struct Routing r;
    uint16_t router_number;
    uint16_t time_interval;
    uint16_t curr_router_id;
    uint16_t curr_router_port;
    uint16_t curr_data_port;
    uint16_t curr_cost;
    uint32_t curr_router_ip;

    struct Timeout router_timeout;



    int overhead = 0;

    memcpy(&router_number, payload + overhead, sizeof(router_number));

    router_number = ntohs(router_number);
    overhead += 2;
    cout << "router_number number: " << router_number << endl;

    memcpy(&time_interval, payload + overhead, sizeof(time_interval));
    time_interval = ntohs(time_interval);
    overhead += 2;
    cout << "time interval: " << time_interval << endl;

    /* init neighbor expire time to inifinity */
    struct timeval tmp_tv;
    gettimeofday(&tmp_tv, NULL);

    for (int i = 0; i < router_number; i++) {

        memcpy(&curr_router_id, payload + overhead, sizeof(curr_router_id));
        curr_router_id = ntohs(curr_router_id);
        overhead += 2;

        memcpy(&curr_router_port, payload + overhead, sizeof(curr_router_port));
        curr_router_port = ntohs(curr_router_port);
        overhead += 2;

        memcpy(&curr_data_port, payload + overhead, sizeof(curr_data_port));
        curr_data_port = ntohs(curr_data_port);
        overhead += 2;

        memcpy(&curr_cost, payload + overhead, sizeof(curr_cost));
        curr_cost = ntohs(curr_cost);
        overhead += 2;

        memcpy(&curr_router_ip, payload + overhead, sizeof(curr_router_ip));
        curr_router_ip = ntohl(curr_router_ip);
        overhead += 4;


        r.id = curr_router_id;
        r.route_port = curr_router_port;
        r.data_port = curr_data_port;
        r.c = curr_cost;
        r.ip = curr_router_ip;
        r.total_cost = curr_cost;

        routing_table[curr_router_id] = r;


        if(curr_cost == 0){

            self.id = curr_router_id;
            self.total_cost = 0;
            self.data_port = curr_data_port;
            self.route_port = curr_router_port;
            self.ip = curr_router_ip;
            self.c = 0;

        }





        /* init timeout list */

    }
//    /* after init, we can set waiting time for select(), at first, wait T and send dv */


    char *ctrl_response_header, *ctrl_response_payload, *ctrl_response;

    int response_len;
    uint16_t payload_len =22;


    ctrl_response_header = create_response_header(sock_index, 0X01, 0X00, payload_len);

    response_len = CONTROL_HEADER_SIZE + payload_len;
    ctrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, payload_len);

    sendALL(sock_index, ctrl_response, response_len);


    router_socket = create_route_sock(self.route_port);
    data_socket = create_data_sock(self.data_port);


    FD_SET(router_socket, &master_list);
    FD_SET(data_socket, &master_list);





//    /* response to controller */

//    /* create router listening socket and data listening socket */




}

