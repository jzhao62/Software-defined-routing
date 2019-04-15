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
#include "../include/header.h"
#include "../include/utils.h"
#include "../include/connection_manager.h"
#include "../include/data_handler.h"
#include <iostream>

using namespace std;

vector<int> control_socket_list;
vector<Route_Neighbor> neighbors;
vector<Routing> table;
vector<Timeout> routers_timeout;
int router_socket;
uint32_t my_ip;
uint16_t my_router_port;
uint16_t my_data_port;
uint16_t my_id;
uint16_t routers_number;
uint16_t time_period;

vector<Transfer_File> transfer_files;
char *last_packet;
char *penultimate_packet;
char *file_buffer;
int datagram_count;

int create_control_sock(uint16_t control_port) {
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
    control_addr.sin_port = htons(control_port);

    if (bind(sock, (struct sockaddr *) &control_addr, sizeof(control_addr)) < 0) ERROR("bind() failed");

    if (listen(sock, 5) < 0) ERROR("listen() failed");

    return sock;
}

int create_route_sock(uint16_t router_port) {
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

int new_control_conn(int sock_index) {
    int fdaccept, caddr_len;
    struct sockaddr_in remote_controller_addr;

    caddr_len = sizeof(remote_controller_addr);
    fdaccept = accept(sock_index, (struct sockaddr *) &remote_controller_addr, (socklen_t *) &caddr_len);
    if (fdaccept < 0) ERROR("accept() failed");

    /* Insert into list of active control connections */
    control_socket_list.push_back(fdaccept);

    FD_SET(fdaccept, &master_list);
    if (fdaccept > head_fd) head_fd = fdaccept;

    return fdaccept;
}

void remove_control_conn(int sock_index) {
    for (int i = 0; i < control_socket_list.size(); i++) {
        if (control_socket_list[i] == sock_index) {
            control_socket_list.erase(control_socket_list.begin() + i);
            break;
        }
    }

    close(sock_index);
}

int new_route_conn(uint32_t remote_ip, uint16_t remote_port, uint16_t remote_id) {
    int sock;
    struct sockaddr_in remote_router_addr;
    socklen_t addrlen = sizeof(remote_router_addr);

    struct Route_Neighbor neighbor;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) ERROR("socket() failed");


//    bzero(&remote_router_addr, sizeof(remote_router_addr));
//
//    remote_router_addr.sin_family = AF_INET;
//    remote_router_addr.sin_addr.s_addr = htonl(remote_ip);
//    remote_router_addr.sin_port = htons(remote_port);
//
//    if (connect(sock, (struct sockaddr *) &remote_router_addr, sizeof(remote_router_addr)) < 0) ERROR(
//            "connect() failed");
    neighbor.router_id = remote_id;
    neighbor.socket = sock;
    neighbor.ip = remote_ip;
    neighbor.port = remote_port;
    neighbors.push_back(neighbor);
    return sock;
}

void remove_route_conn(uint16_t router_id) {
    for (int i = 0; i < neighbors.size(); i++) {
        if (neighbors[i].router_id == router_id) {
            cout << "remove router " << router_id << " from neighbors list!" << endl;
            close(neighbors[i].socket);
            neighbors.erase(neighbors.begin() + i);
            break;
        }
    }
}

bool isControl(int sock_index) {
    vector<int>::iterator it = find(control_socket_list.begin(), control_socket_list.end(), sock_index);
    return it != control_socket_list.end();
}

bool control_recv_hook(int sock_index) {
    char *cntrl_header, *cntrl_payload;
    uint8_t control_code;
    uint16_t payload_len;

    /* Get control header */
    cntrl_header = (char *) malloc(sizeof(char) * CONTROL_HEADER_SIZE);
    bzero(cntrl_header, CONTROL_HEADER_SIZE);

    if (recvALL(sock_index, cntrl_header, CONTROL_HEADER_SIZE) < 0) {
        cout << "close control connection" << endl;
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

    cout << "receive controller message: " << (int) control_code << endl;
    /* Triage on control_code */
    switch (control_code) {
        case AUTHOR:
            author(sock_index);
            break;
        case INIT:
            init(sock_index, cntrl_payload);
            break;
        case ROUTING_TABLE:
            routing_table(sock_index);
            break;
        case UPDATE:
            update(sock_index, cntrl_payload);
            break;
        case CRASH:
            crash(sock_index);
            break;
        case SENDFILE:
            send_file(sock_index, cntrl_payload, payload_len);
            break;
        case SENDFILE_STATS:
            send_file_stats(sock_index, cntrl_payload);
            break;
        case LAST_DATA_PACKET:
            last_data_packet(sock_index);
            break;
        case PENULTIMATE_DATA_PACKET:
            penultimate_data_packet(sock_index);
            break;

        default:
            break;
    }

    if (payload_len != 0)
        free(cntrl_payload);

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

void init(int sock_index, char *payload) {
    char *ctrl_response;
//    char *init_payload;
    struct Routing routing_content;
    uint16_t routers, time_interval;
    uint16_t router_id, router_port, data_port, cost;
    uint32_t router_ip;
    struct Timeout router_timeout;

//    init_payload = (char *) malloc(sizeof(char) * INIT_PAYLOAD_SIZE);
//    bzero(init_payload, INIT_PAYLOAD_SIZE);

    int head = 0;

    /* following is read information from payload */
    memcpy(&routers, payload + head, sizeof(routers));
    routers = ntohs(routers);
    head += 2;
    cout << "routers number: " << routers << endl;

    memcpy(&time_interval, payload + head, sizeof(time_interval));
    time_interval = ntohs(time_interval);
    head += 2;
    cout << "time interval: " << time_interval << endl;

    routers_number = routers;
    time_period = time_interval;

    /* init neighbor expire time to inifinity */
    struct timeval tmp_tv;
    gettimeofday(&tmp_tv, NULL);
    tmp_tv.tv_sec += 1000 * time_period;

    for (int i = 0; i < routers_number; i++) {
//        memcpy(init_payload, payload + head, INIT_PAYLOAD_SIZE);

        memcpy(&router_id, payload + head, sizeof(router_id));
        router_id = ntohs(router_id);
        head += 2;
//        cout << "router id: " << router_id << endl;

        memcpy(&router_port, payload + head, sizeof(router_port));
        router_port = ntohs(router_port);
        head += 2;
//        cout << "router port: " << router_port << endl;

        memcpy(&data_port, payload + head, sizeof(data_port));
        data_port = ntohs(data_port);
        head += 2;
//        cout << "data port: " << data_port << endl;

        memcpy(&cost, payload + head, sizeof(cost));
        cost = ntohs(cost);
        head += 2;
//        cout << "cost: " << cost << endl;

        memcpy(&router_ip, payload + head, sizeof(router_ip));
        router_ip = ntohl(router_ip);
        head += 4;
//        cout << "router ip: " << router_ip << endl;

        if (cost == 0) { // cost=0 means this is me
            my_id = router_id;
            my_router_port = router_port;
            my_data_port = data_port;
            my_ip = router_ip;
            cout << "my router id: " << my_id << endl;

            /* this is request */
            routing_content.next_hop_id = my_id;
        } else {
            if (cost < INF) {// neighbors
                routing_content.next_hop_id = router_id;
                /* create udp sockets connecting to neighbors */
                new_route_conn(router_ip, router_port, router_id);
            } else {// unreachable
                routing_content.next_hop_id = INF;
            }
        }
        /*add info into routing table*/
        routing_content.dest_id = router_id;
        routing_content.dest_route_port = router_port;
        routing_content.dest_data_port = data_port;
        routing_content.dest_cost = cost;
        routing_content.dest_ip = router_ip;
        routing_content.path_cost = routing_content.dest_cost;
        table.push_back(routing_content);
//        bzero(init_payload, INIT_PAYLOAD_SIZE);

        /* init timeout list */
        router_timeout.router_id = router_id;
        router_timeout.is_connected = false;
        router_timeout.expired_time = tmp_tv;
        routers_timeout.push_back(router_timeout);

    }
    /* after init, we can set waiting time for select(), at first, wait T and send dv */
    first_time = false;
    gettimeofday(&next_send_time, NULL);
    next_send_time.tv_sec += time_period;
    tv.tv_sec = time_period;
    tv.tv_usec = 0;

    /* response to controller */
    ctrl_response = create_response_header(sock_index, 1, 0, 0);
    sendALL(sock_index, ctrl_response, CONTROL_HEADER_SIZE);

    /* create router listening socket and data listening socket */
    cout << "creating router socket..." << endl;
    router_socket = create_route_sock(my_router_port);
    FD_SET(router_socket, &master_list);
    if (router_socket > head_fd) head_fd = router_socket;

    cout << "creating data socket..." << endl;
    data_socket = create_data_sock(my_data_port);
    FD_SET(data_socket, &master_list);
    if (data_socket > head_fd) head_fd = data_socket;

    next_event_time = next_send_time; // init next_event_time, assume init() called only once++++++++++

    /* malloc last_packet and penultimate_packet */
    last_packet = (char *) malloc(sizeof(char) * DATA_PAYLOAD);
    bzero(last_packet, DATA_PAYLOAD + 12);
    penultimate_packet = (char *) malloc(sizeof(char) * DATA_PAYLOAD);
    bzero(penultimate_packet, DATA_PAYLOAD + 12);
    file_buffer = (char *) malloc(sizeof(char) * 10000 * DATA_PAYLOAD);
    bzero(file_buffer, 10000 * DATA_PAYLOAD);

    datagram_count = 0;
}

void routing_table(int sock_index) {
    uint16_t payload_len, response_len;
    char *ctrl_response_header, *ctrl_response_payload, *ctrl_response;

    payload_len = table.size() * ROUTING_TABLE_CONTENT_SIZE;
    ctrl_response_payload = (char *) malloc(sizeof(char) * payload_len);
    bzero(ctrl_response_payload, payload_len);

    int offset = 0;
    uint16_t id, next_hop, cost;
    /* copy routing table to payload */
    for (int i = 0; i < table.size(); i++) {
        id = htons(table[i].dest_id);
        memcpy(ctrl_response_payload + offset, &id, sizeof(id));
        offset += 4; // id + padding

        next_hop = htons(table[i].next_hop_id);
        memcpy(ctrl_response_payload + offset, &next_hop, sizeof(next_hop));
        offset += 2;

        cost = htons(table[i].dest_cost);
        memcpy(ctrl_response_payload + offset, &cost, sizeof(cost));
        offset += 2;
    }

    /* generate succeed header */
    ctrl_response_header = create_response_header(sock_index, 2, 0, payload_len);

    response_len = CONTROL_HEADER_SIZE + payload_len;
    ctrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, payload_len);

    sendALL(sock_index, ctrl_response, response_len);
}

void update(int sock_index, char *payload) {
    char *ctrl_response;

    uint16_t router_id, cost;

    /* get info from payload */
    memcpy(&router_id, payload, sizeof(router_id));
    router_id = ntohs(router_id);
    memcpy(&cost, payload + 2, sizeof(cost));
    cost = ntohs(cost);

    for (int i = 0; i < table.size(); i++) {
        if (table[i].dest_id == router_id) {
            /* update cost of router path, NOT SURE whether we only need to update one cost */
            table[i].dest_cost = cost;
//            for (int j = 0; j < routers_timeout.size(); j++) {
//                if (routers_timeout[j].router_id == router_id) {
//                    /*  */
//                    if (cost == INF) {
//                        routers_timeout[j].is_connected = false;
//                    } else {
//                        routers_timeout[j].is_connected = true;
//                    }
//                }
//            }
            break;
        }
    }

    ctrl_response = create_response_header(sock_index, 3, 0, 0);
    sendALL(sock_index, ctrl_response, CONTROL_HEADER_SIZE);
}

void crash(int sock_index) {
    char *ctrl_response;

    ctrl_response = create_response_header(sock_index, 4, 0, 0);
    sendALL(sock_index, ctrl_response, CONTROL_HEADER_SIZE);

    cout << "close all sockets!" << endl;
    // route sockets
    for (int i = 0; i < neighbors.size(); i++) {
        close(neighbors[i].socket);
    }
    close(router_socket);

    // data sockets
    close_all_data_sock();
    close(data_socket);

    // control sockets
    for (int i = 0; i < control_socket_list.size(); i++) {
        close(control_socket_list[i]);
    }
    close(control_socket);

    exit(0);
}

void send_file(int sock_index, char *payload, uint16_t payload_len) {
    uint32_t dest_ip;
    uint8_t ttl;
    uint8_t transfer_id;
    uint16_t seq_num;
    char *file_name;

    file_name = (char *) malloc(sizeof(char) * (payload_len - 8));

    int offset = 0;
    memcpy(&dest_ip, payload + offset, sizeof(dest_ip));
    dest_ip = ntohl(dest_ip);
    offset += 4;

    memcpy(&ttl, payload + offset, sizeof(ttl));
    offset += 1;

    memcpy(&transfer_id, payload + offset, sizeof(transfer_id));
    offset += 1;

    memcpy(&seq_num, payload + offset, sizeof(seq_num));

    /* find or create socket */
    uint16_t next_hop = 0;
    uint32_t next_hop_ip = 0;
    for (int i = 0; i < table.size(); i++) {
        if (table[i].dest_ip == dest_ip) {
            next_hop = table[i].next_hop_id;
            break;
        }
    }
    if (next_hop == 0) {
        cout << "not find next hop, impossible" << endl;
    }
    for (int j = 0; j < table.size(); j++) {
        if (table[j].dest_id == next_hop) {
            next_hop_ip = table[j].dest_ip;
            break;
        }
    }
    if (next_hop_ip == 0) {
        cout << "not find next hop ip, impossible" << endl;
    }

    int link_socket;
    link_socket = get_data_sock(next_hop_ip);


    FILE *fr = fopen(file_name, "r");

    char buffer[DATA_PAYLOAD];
    char *packet;
    int length = 0;
    int fin;
    while ((length = fread(buffer, sizeof(char), DATA_PAYLOAD, fr)) > 0) {
        /* set fin */
        if ((length = fread(buffer, sizeof(char), DATA_PAYLOAD, fr)) > 0) {
            fin = 0;
            fseek(fr, -length, SEEK_CUR);
        } else {
            fin = 1;
        }
        packet = (char *) malloc(sizeof(char) * (DATA_PAYLOAD + 12));
        packet = create_data_packet(dest_ip, transfer_id, ttl, seq_num++, fin, buffer);
        if (sendALL(link_socket, packet, DATA_PAYLOAD + 12) < 0) {
            printf("Send File:%s Failed./n", file_name);
            break;
        }
        bzero(buffer, DATA_PAYLOAD);
        bzero(packet, DATA_PAYLOAD + 12);
    }

    /* response */
    char *ctrl_response;
    ctrl_response = create_response_header(sock_index, 5, 0, 0);

    sendALL(sock_index, ctrl_response, CONTROL_HEADER_SIZE);

}

void send_file_stats(int sock_index, char *payload) {
    uint8_t transfer_id;
    uint16_t payload_len, response_len;

    memcpy(&transfer_id, payload, sizeof(transfer_id));

    int seq_amount = 0;
    int id_pos = -1;
    uint8_t ttl = 0;

    for (int i = 0; i < transfer_files.size(); i++) {
        if (transfer_files[i].transfer_id == transfer_id) {
            seq_amount = transfer_files[i].sequence.size();
            ttl = transfer_files[i].ttl;
            id_pos = i;
            break;
        }
    }

    if (id_pos < 0) {
        cout << "didn't find transfer id" << endl;
    }

    payload_len = 2 * seq_amount + 4;
    response_len = CONTROL_HEADER_SIZE + payload_len;
    char *ctrl_response_payload, *ctrl_response_header, *ctrl_response;

    ctrl_response = (char *) malloc(sizeof(char) * response_len);

    ctrl_response_header = create_response_header(sock_index, 6, 0, payload_len);

    ctrl_response_payload = (char *) malloc(sizeof(char) * payload_len);
    bzero(ctrl_response_payload, payload_len);
    int offset = 0;

    memcpy(ctrl_response_payload + offset, &transfer_id, sizeof(transfer_id));
    offset += 1;

    memcpy(ctrl_response_payload + offset, &ttl, sizeof(ttl));
    offset += 3;

    uint16_t seq_n;
    for (int i = 0; i < seq_amount; i++) {
        seq_n = transfer_files[id_pos].sequence[i];
        seq_n = htons(seq_n);
        memcpy(ctrl_response_payload + offset, &seq_n, sizeof(uint16_t));
        offset += 1;
    }

    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, payload_len);

    sendALL(sock_index, ctrl_response, response_len);

}

void last_data_packet(int sock_index) {
    uint16_t response_len;
    char *ctrl_response_header, *ctrl_response_payload, *ctrl_response;

    ctrl_response_payload = (char *) malloc(DATA_PAYLOAD);
    memcpy(ctrl_response_payload, last_packet, DATA_PAYLOAD);

    ctrl_response_header = create_response_header(sock_index, 7, 0, DATA_PAYLOAD);

    response_len = CONTROL_HEADER_SIZE + DATA_PAYLOAD;
    ctrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, DATA_PAYLOAD);

    sendALL(sock_index, ctrl_response, response_len);
}

void penultimate_data_packet(int sock_index) {
    uint16_t response_len;
    char *ctrl_response_header, *ctrl_response_payload, *ctrl_response;

    ctrl_response_payload = (char *) malloc(DATA_PAYLOAD);
    memcpy(ctrl_response_payload, penultimate_packet, DATA_PAYLOAD);

    ctrl_response_header = create_response_header(sock_index, 8, 0, DATA_PAYLOAD);

    response_len = CONTROL_HEADER_SIZE + DATA_PAYLOAD;
    ctrl_response = (char *) malloc(response_len);
    /* Copy Header */
    memcpy(ctrl_response, ctrl_response_header, CONTROL_HEADER_SIZE);
    /* Copy Payload */
    memcpy(ctrl_response + CONTROL_HEADER_SIZE, ctrl_response_payload, DATA_PAYLOAD);

    sendALL(sock_index, ctrl_response, response_len);
}

//void update_routing_table(int sock_index) {
//    char *routing_header, *payload;
//    struct sockaddr_in route_addr;
//    uint16_t update_num, src_router_port;
//    uint32_t src_ip;
//    uint16_t received_router_id = INF;
//    int received_router_pos;
//
//    socklen_t addr_len = sizeof(struct sockaddr_in);
//    route_addr.sin_family = AF_INET;
//    route_addr.sin_port = htons(my_router_port);
//    route_addr.sin_addr.s_addr = htonl(INADDR_ANY); // don't need to assign a special ip
//
//    /* first read header */
//    routing_header = (char *) malloc(sizeof(char) * ROUTING_HEADER_SIZE);
//    bzero(routing_header, ROUTING_HEADER_SIZE);
//
//    if (recvfrom(sock_index, routing_header, ROUTING_HEADER_SIZE, 0, (struct sockaddr *) &route_addr, &addr_len) < 0) {
//        cout << "receive routing message fail!" << endl;
//        return;
//    }
//
//    int offset = 0;
//
//    /* number of update fields */
//    memcpy(&update_num, routing_header + offset, sizeof(update_num));
//    update_num = ntohs(update_num);
//    offset += 2;
//
//    /* source router port */
//    memcpy(&src_router_port, routing_header + offset, sizeof(src_router_port));
//    src_router_port = ntohs(src_router_port);
//    offset += 2;
//
//    /* source router ip */
//    memcpy(&src_ip, routing_header + offset, sizeof(src_ip));
//    src_ip = ntohl(src_ip);
//
//    for (int i = 0; i < table.size(); i++) { // find id of received router
//        if (table[i].dest_ip == src_ip) {
//            received_router_id = table[i].dest_id;
//            received_router_pos = i;
//            cout << "receiving dv from router " << received_router_id << "..." << endl;
//            break;
//        }
//    }
//
//    if (received_router_id == INF) {
//        cout << "can't find router from routing table, this should not happen" << endl;
//    }
//    if (table[received_router_pos].dest_cost == INF) {
//        cout << "it's not possible to receive routing packet from non-neighbors"
//             << endl;
//        cout << "this may be the delay dv from neighbor, but it doesn't matter, we return" << endl;
//    }
//
//    if (update_num > 0) { // malloc payload, always goes into
//        payload = (char *) malloc(sizeof(char) * update_num * ROUTING_CONTENT_SIZE);
//    }
//
//    if (payload != NULL) { // always executes
//        /* receive dv payload */
//        if (recvfrom(sock_index, payload, (size_t) ROUTING_CONTENT_SIZE * update_num, 0,
//                     (struct sockaddr *) &route_addr, &addr_len) < 0) {
//            cout << "receive routing message fail!" << endl;
//            return;
//        }
//
//        if (table[received_router_pos].dest_cost == INF) { // to free the socket buffer
//            return;
//        }
//
//        offset = 0;
//        uint32_t remote_router_ip;
//        uint16_t remote_router_port, remote_router_id, cost;
//        /* read payload and update routing table */
//        for (int i = 0; i < update_num; i++) {
//            memcpy(&remote_router_ip, payload + offset, sizeof(remote_router_ip));
//            remote_router_ip = ntohl(remote_router_ip);
//            offset += 4;
//
//            memcpy(&remote_router_port, payload + offset, sizeof(remote_router_port));
//            remote_router_port = ntohs(remote_router_port);
//            offset += 4;
//
//            memcpy(&remote_router_id, payload + offset, sizeof(remote_router_id));
//            remote_router_id = ntohs(remote_router_id);
//            offset += 2;
//
//            memcpy(&cost, payload + offset, sizeof(cost));
//            cost = ntohs(cost);
//            offset += 2;
//
//            if (cost == INF) { // if cost is infinity, there is no need to update anything
//                continue;
//            }
//            for (int j = 0; j < table.size(); j++) {
//                /* D_x(y) = min(d_x(v) + d_v(y)), v is this neighbor */
//                if (table[j].dest_id == remote_router_id) {
//                    if (table[i].dest_cost > table[received_router_pos].dest_cost + cost) {
//                        /* if updated, the next hop from x to y is v */
//                        table[i].dest_cost = table[received_router_pos].dest_cost + cost;
//                        table[i].next_hop_id = received_router_id;
//                    }
//                    break;
//                }
//            }
//        }
//    }
//
//    // update timeout list
//    struct timeval cur_tv;
//    gettimeofday(&cur_tv, NULL);
//    cout << "time of receiving router packet from router " << received_router_id << " is " << cur_tv.tv_sec << endl;
//    cur_tv.tv_sec += 3 * time_period;
//
//    for (int i = 0; i < routers_timeout.size(); i++) {
//        if (routers_timeout[i].router_id == received_router_id) {
//            cout << "expired time of router " << received_router_id << " is set from "
//                 << routers_timeout[i].expired_time.tv_sec;
//            /* since we receive dv from this neighbor, we need to update the expired time */
//            routers_timeout[i].expired_time = cur_tv;
//            routers_timeout[i].is_connected = true;
//            cout << " to " << routers_timeout[i].expired_time.tv_sec << endl;
//            break;
//        }
//    }
//}

void update_routing_table(int sock_index) {
    char *payload;
    struct sockaddr_in route_addr;
    uint16_t update_num, src_router_port;
    uint32_t src_ip;
    uint16_t received_router_id = INF;
    int received_router_pos;

    socklen_t addr_len = sizeof(struct sockaddr_in);
    route_addr.sin_family = AF_INET;
    route_addr.sin_port = htons(my_router_port);
    route_addr.sin_addr.s_addr = htonl(INADDR_ANY); // don't need to assign a special ip

    update_num = (uint16_t) table.size();

    int offset = 0;


    payload = (char *) malloc(sizeof(char) * (ROUTING_HEADER_SIZE + ROUTING_CONTENT_SIZE * update_num));

    if (payload != NULL) { // always executes
        /* receive dv payload */
        if (recvfrom(sock_index, payload, (size_t) ROUTING_HEADER_SIZE + ROUTING_CONTENT_SIZE * update_num, 0,
                     (struct sockaddr *) &route_addr, &addr_len) < 0) {
            cout << "receive routing message fail!" << endl;
            return;
        }

        /* number of update fields */
        memcpy(&update_num, payload + offset, sizeof(update_num));
        update_num = ntohs(update_num);
        offset += 2;

        /* source router port */
        memcpy(&src_router_port, payload + offset, sizeof(src_router_port));
        src_router_port = ntohs(src_router_port);
        offset += 2;

        /* source router ip */
        memcpy(&src_ip, payload + offset, sizeof(src_ip));
        src_ip = ntohl(src_ip);
        offset += 4;

        for (int i = 0; i < table.size(); i++) { // find id of received router
            if (table[i].dest_ip == src_ip) {
                received_router_id = table[i].dest_id;
                received_router_pos = i;
                cout << "receiving dv from router " << received_router_id << "..." << endl;
                break;
            }
        }

        if (received_router_id == INF) {
            cout << "can't find router from routing table, this should not happen" << endl;
        }
        if (table[received_router_pos].dest_cost == INF) {
            cout << "it's not possible to receive routing packet from non-neighbors"
                 << endl;
            cout << "this may be the delay dv from neighbor, but it doesn't matter, we return" << endl;
        }

        if (table[received_router_pos].dest_cost == INF) { // to free the socket buffer
            return;
        }

        table[received_router_pos].remote_dv.clear();
        uint32_t remote_router_ip;
        uint16_t remote_router_port, remote_router_id, cost;
        struct Remote_DV dv;

        /* read payload and update routing table */
        for (int i = 0; i < update_num; i++) {
            memcpy(&remote_router_ip, payload + offset, sizeof(remote_router_ip));
            remote_router_ip = ntohl(remote_router_ip);
            offset += 4;

            memcpy(&remote_router_port, payload + offset, sizeof(remote_router_port));
            remote_router_port = ntohs(remote_router_port);
            offset += 4;

            memcpy(&remote_router_id, payload + offset, sizeof(remote_router_id));
            remote_router_id = ntohs(remote_router_id);
            offset += 2;

            memcpy(&cost, payload + offset, sizeof(cost));
            cost = ntohs(cost);
            offset += 2;

//            if (cost == INF) { // if cost is infinity, there is no need to update anything
//                if(remote_router_id == my_id){
//                    table[received_router_pos].dest_cost = INF;
//                }else{
//                    continue;
//                }
//            }
//            for (int j = 0; j < table.size(); j++) {
//                /* D_x(y) = min(d_x(v) + d_v(y)), v is this neighbor */
//                if (table[j].dest_id == remote_router_id) {
//                    if (table[i].dest_cost > table[received_router_pos].dest_cost + cost) {
//                        /* if updated, the next hop from x to y is v */
//                        table[i].dest_cost = table[received_router_pos].dest_cost + cost;
//                        table[i].next_hop_id = received_router_id;
//                    }
//                    break;
//                }
//            }

            dv.dest_id = remote_router_id;
            dv.cost = cost;
            table[received_router_pos].remote_dv.push_back(dv);
            if (remote_router_id == my_id) {
                table[received_router_pos].dest_cost = cost;
                continue;
            }

        }
        for (int i = 0; i < table.size(); i++) {
            if (table[i].dest_id == my_id) {
                continue;
            }
            for (int j = 0; j < table[received_router_pos].remote_dv.size(); j++) {
                if (table[received_router_pos].remote_dv[j].dest_id == table[i].dest_id) {
                    /* d_v(y) */
                    table[i].dest_cost =
                            table[received_router_pos].dest_cost + table[received_router_pos].remote_dv[j].cost;
                    table[i].next_hop_id = received_router_id;

                    for (int k = 0; k < table.size(); k++) {
                        if (table[k].remote_dv.empty()) {
                            continue;
                        }
                        for (int l = 0; l < table[k].remote_dv.size(); l++) {
                            if (table[k].remote_dv[l].dest_id == table[i].dest_id) {
                                if (table[i].dest_cost > table[k].dest_cost + table[k].remote_dv[j].cost) {
                                    table[i].dest_cost = table[k].dest_cost + table[k].remote_dv[j].cost;
                                    table[i].next_hop_id = table[k].dest_id;
                                }
                                break;
                            }
                        }
                    }

                    break;
                }
            }
        }
    }

    // update timeout list
    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);
    cout << "time of receiving router packet from router " << received_router_id << " is " << cur_tv.tv_sec << endl;
    cur_tv.tv_sec += 3 * time_period;

    for (int i = 0; i < routers_timeout.size(); i++) {
        if (routers_timeout[i].router_id == received_router_id) {
            cout << "expired time of router " << received_router_id << " is set from "
                 << routers_timeout[i].expired_time.tv_sec;
            /* since we receive dv from this neighbor, we need to update the expired time */
            routers_timeout[i].expired_time = cur_tv;
            routers_timeout[i].is_connected = true;
            cout << " to " << routers_timeout[i].expired_time.tv_sec << endl;
            break;
        }
    }
}

void send_dv() {
    cout << "send dv to neighbors!" << endl;
    char *dv;
    dv = create_distance_vector(); // generate dv packet
    int update_num = (int) table.size();
    for (int i = 0; i < neighbors.size(); i++) {
        cout << "send dv to router " << neighbors[i].router_id << endl;
        if (send_udp(neighbors[i].socket, dv, ROUTING_HEADER_SIZE + update_num * ROUTING_CONTENT_SIZE, neighbors[i].ip,
                     neighbors[i].port) < 0) {
            cout << "Send DV fail" << endl;
        }
    }
}

char *create_distance_vector() {
    char *buffer, *header, *payload;
    struct Route_Header *dv_header;
    uint16_t update_num = (uint16_t) table.size(); // number of updating fields, should be 5 in this project

    /* buffer: dv, size: header + num * content_size */
    buffer = (char *) malloc(sizeof(char) * (ROUTING_HEADER_SIZE + update_num * ROUTING_CONTENT_SIZE));
    bzero(buffer, (size_t) (ROUTING_HEADER_SIZE + update_num * ROUTING_CONTENT_SIZE));

    /*header: dv header, size: 8*/
    header = (char *) malloc(sizeof(char) * ROUTING_HEADER_SIZE);
    dv_header = (struct Route_Header *) header;
    dv_header->num_update = htons(update_num);
    dv_header->source_port = htons(my_router_port);
    dv_header->source_ip = htonl(my_ip);
    memcpy(buffer, header, ROUTING_HEADER_SIZE);

    /* payload: dv payload, size: num * 12 */
    payload = (char *) malloc(sizeof(char) * (update_num * ROUTING_CONTENT_SIZE));
    bzero(payload, (size_t) (update_num * ROUTING_CONTENT_SIZE));

    /* copy content to payload */
    int offset = 0;
    uint32_t ip;
    uint16_t port, id, cost;
    for (int i = 0; i < update_num; i++) { // again, there is update_num of content to be added
        ip = htonl(table[i].dest_ip);
        memcpy(payload + offset, &ip, sizeof(ip));
        offset += 4;

        port = htons(table[i].dest_route_port);
        memcpy(payload + offset, &port, sizeof(port));
        offset += 4; // port + padding

        id = htons(table[i].dest_id);
        memcpy(payload + offset, &id, sizeof(id));
        offset += 2;

        cost = htons(table[i].dest_cost);
        memcpy(payload + offset, &cost, sizeof(cost));
        offset += 2;
    }
    memcpy(buffer + ROUTING_HEADER_SIZE, payload, (size_t) update_num * ROUTING_CONTENT_SIZE);

    return buffer;
}

char *create_data_packet(uint32_t dest_ip, uint8_t transfer_id, uint8_t ttl, uint16_t seq_num, int fin, char *payload) {
    char *buffer;
    buffer = (char *) malloc(sizeof(char) * (DATA_PAYLOAD + 12));
    bzero(buffer, DATA_PAYLOAD + 12);

    int offset = 0;
    uint32_t ip = htonl(dest_ip);
    uint16_t
            num = htons(seq_num);

    memcpy(buffer + offset, &ip, sizeof(ip));
    offset += 4;

    memcpy(buffer + offset, &transfer_id, sizeof(transfer_id));
    offset += 1;

    memcpy(buffer + offset, &ttl, sizeof(ttl));
    offset += 1;

    memcpy(buffer + offset, &num, sizeof(num));
    offset += 2;

    uint32_t fin_padding;
    if (fin == 1) {
        fin_padding = LAST_DATA_PACKET;
    } else {
        fin_padding = NOT_LAST_PACKET;
    }
    memcpy(buffer + offset, &fin_padding, sizeof(fin_padding));
    offset += 4;

    memcpy(buffer + offset, payload, DATA_PAYLOAD);

    return buffer;
}