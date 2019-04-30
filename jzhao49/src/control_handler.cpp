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
#include <climits>




using namespace std;

vector<int> control_conn_list;
map<uint16_t , routing_packet > neighbors;


routing_packet self;

map<uint16_t , uint16_t > DV;


// key : destination, val : next_hop starting from current node
map<uint16_t , uint16_t > next_hops;

map<uint16_t , router*> all_nodes;


vector<pair<uint16_t, uint32_t >> immediate_neighbors;


uint16_t router_number;



 uint16_t my_router_port;


UDPSocket *router_sock;




int router_socket;
uint16_t time_period;




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

int create_route_sock(uint16_t router_port){
    int sock;
    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) ERROR("socket() failed");

    /* Make socket re-usable */
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(opt)) < 0) ERROR("setsockopt() failed");

    bzero(&router_addr, sizeof(router_addr));

    router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    router_addr.sin_port = htons(router_port);

    if (bind(sock, (struct sockaddr *) &router_addr, sizeof(router_addr)) < 0) ERROR("bind() failed");

    return sock;
}

int create_data_sock(uint16_t data_sock){
    int sock;
    struct sockaddr_in router_addr;
    socklen_t addrlen = sizeof(router_addr);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) ERROR("socket() failed");

    /* Make socket re-usable */
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void *) &opt, sizeof(opt)) < 0) ERROR("setsockopt() failed");

    bzero(&router_addr, sizeof(router_addr));

    router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    router_addr.sin_port = htons(data_sock);

    if (bind(sock, (struct sockaddr *) &router_addr, sizeof(router_addr)) < 0) ERROR("bind() failed");

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
            routing(sock_index);
            break;
        case UPDATE:
            update(sock_index, cntrl_payload);

            break;
        case CRASH:
            break;
        case SENDFILE:
            sendfile(sock_index, cntrl_payload);
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


    // length of the AUTHOR statement
    payload_len = sizeof(AUTHOR_STATEMENT) - 1; // Discount the NULL character
    ctrl_response_payload = (char *) malloc(payload_len);

    memcpy(ctrl_response_payload, AUTHOR_STATEMENT, payload_len);
    cout << AUTHOR_STATEMENT << endl;

    ctrl_response_header = create_response_header(sock_index, 0, 0, payload_len);


    // length of the entire response
    response_len = CONTROL_HEADER_SIZE + payload_len;
    ctrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, payload_len);

    sendALL(sock_index, ctrl_response, response_len);
}

/**
 * Contains the data required to initialize a router.
 * It lists the number of routers in the network, their IP address, router and data port numbers, and, the initial costs of the links to all routers.
 * It also contains the periodic interval (in seconds) for the routing updates to be broadcast to all neighbors (routers directly connected).

Note that the link cost value to all the routers, except the neighbors, will be INF (infinity). Further, there will be an entry to self as well with cost set to 0.

The router, after receiving this message, should start broadcasting the routing updates and performing other operations required by the distance vector protocol, after one periodic interval.
 * @param sock_index
 * @param payload
 */
void initialize(int sock_index, char* payload){


    struct routing_packet r;
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


    time_period = time_interval;


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

        cout << curr_router_ip << endl;


        overhead += 4;


        r.router_id = curr_router_id;
        r.router_port = curr_router_port;
        r.data_port = curr_data_port;
        r.cost_from_source = curr_cost;
        r.ip = curr_router_ip;


        if(curr_cost == 65535) continue;

        neighbors[curr_router_id] = r;

        all_nodes[curr_router_id] = new router(r.router_id,r.ip, r.router_port, r.data_port, 1);



        if(curr_cost == 0){
            self.router_id= curr_router_id;
            self.data_port = curr_data_port;
            self.router_port = curr_router_port;
            self.ip = curr_router_ip;
            self.cost_from_source = 0;
            my_router_port = curr_router_port;
            continue;
        }


        immediate_neighbors.push_back({curr_router_port,curr_router_ip});


        /* init timeout list */

    }


    initialize_dv(DV, next_hops, int(self.router_id), int(router_number));



    map<uint16_t ,uint16_t > *tmp_dv = &DV;


    for(auto n : neighbors){

        int id = int(n.first);
        int cost = int(n.second.cost_from_source);
        next_hops[id] = id;

        DV[id] = cost;
    }


    display_DV(DV, next_hops);


    char *ctrl_response_header, *ctrl_response_payload, *ctrl_response;

    int response_len;
    uint16_t payload_len = 0;


    ctrl_response_header = create_response_header(sock_index, 0X01, 0X00, payload_len);

    response_len = CONTROL_HEADER_SIZE + payload_len;

    ctrl_response = (char *) malloc(response_len);
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, payload_len);
    sendALL(sock_index, ctrl_response, response_len);


    router_socket = create_route_sock(self.router_port);

    FD_SET(router_socket, &master_list);
    if(router_socket > head_fd) head_fd = router_socket;

    data_socket = create_data_sock(self.data_port);

    FD_SET(data_socket, &master_list);
    if(data_socket > head_fd) head_fd = data_socket;





    gettimeofday(&next_send_time, NULL);
    next_send_time.tv_sec += time_period;

    tv.tv_sec = time_period;
    tv.tv_usec = 0;

    first_time = false;


}

/**
 * The controller uses this to request the current routing/forwarding table from a given router.
 * The table sent as a response should contain an entry for each router in the network (including self) consisting of the next hop router ID (on the least cost path to that router) and the cost of the path to it.
 */


void routing(int sock_index){
    uint16_t payload_len;
    uint16_t response_len;
    char *ctrl_response_header;
    char *ctrl_response_payload;
    char *ctrl_response;

    payload_len = create_ROUTING_reponse_payload(ctrl_response_payload, DV,next_hops);


    ctrl_response_header = create_response_header(sock_index, 0, 0x02, payload_len);



    response_len = CONTROL_HEADER_SIZE + payload_len;


    ctrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, payload_len);


    int ret = sendALL(sock_index, ctrl_response, response_len);


    if(ret == -1) ERROR("FAILED to send routing response")

}



void update(int sock_index, char* payload){

    uint16_t target_id;
    uint16_t cost;


    unsigned int byte = 0;

    memcpy(&target_id, payload + byte, sizeof(target_id));  byte+=2;
    memcpy(&cost, payload + byte, sizeof(cost)); byte +=2;

    target_id = ntohs(target_id);
    cost = ntohs(cost);


    cout << " update toward " << ntohs(target_id) << " with cost " << ntohs(cost) << endl;


    // reinitliaze local routerr

    neighbors[target_id].cost_from_source = cost;


    initialize_dv(DV, next_hops, int(self.router_id), int(router_number));



    map<uint16_t ,uint16_t > *tmp_dv = &DV;


    for(auto n : neighbors){

        int id = int(n.first);
        int cost = int(n.second.cost_from_source);
        next_hops[id] = id;

        DV[id] = cost;
    }


    display_DV(DV, next_hops);











    char* ctrl_response_header;
    uint16_t payload_len= 0;
    ctrl_response_header = create_response_header(sock_index, 0, 0x03, payload_len);


    int ret = sendALL(sock_index, ctrl_response_header, 8);




}



void crash(int sock_index){

    for(auto a : neighbors){
        a.second.cost_from_source = INF;
    }


    initialize_dv(DV, next_hops, int(self.router_id), int(router_number));

    map<uint16_t ,uint16_t > *tmp_dv = &DV;


    for(auto n : neighbors){

        int id = int(n.first);
        int cost = int(n.second.cost_from_source);
        next_hops[id] = id;

        DV[id] = cost;
    }


    display_DV(DV, next_hops);








}


void sendfile(int sock_index, char* cntrl_payload){
    uint32_t destination_ip;
    uint8_t init_TTL;
    uint8_t transfer_id;
    uint16_t init_sequence_number;




    int byte = 0;

    memcpy(&destination_ip, cntrl_payload + byte, sizeof(destination_ip)); byte += 4;
    memcpy(&init_TTL, cntrl_payload + byte, sizeof(init_TTL)); byte += 1;
    memcpy(&transfer_id, cntrl_payload + byte, sizeof(transfer_id)); byte += 1;
    memcpy(&init_sequence_number, cntrl_payload + byte, sizeof(init_sequence_number)); byte += 2;


    destination_ip = ntohl(destination_ip);
    init_sequence_number = ntohs(init_sequence_number);

    cout << destination_ip << " " << init_TTL << " " << transfer_id << " " << init_sequence_number << endl;

    char* p = cntrl_payload + byte;


    cout << " file name  " << p << endl;











    char* ctrl_response_header;
    uint16_t payload_len= 0;
    ctrl_response_header = create_response_header(sock_index, 0, 0x05, payload_len);


    int ret = sendALL(sock_index, ctrl_response_header, 8);


}

