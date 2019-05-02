
#ifndef PROJECT_CONTROL_HEADER_H
#define PROJECT_CONTROL_HEADER_H

#define CONTROL_HEADER_SIZE 8
#define INIT_PAYLOAD_SIZE 12
#define ROUTING_HEADER_SIZE 8
#define ROUTING_CONTENT_SIZE 12




#define ROUTING_TABLE_CONTENT_SIZE 8
#define DATA_HEADER_SIZE 12
#define CONTROL_CODE_OFFSET 0x04
#define PAYLOAD_LEN_OFFSET 0x06


#define LAST_PACKET 0x8000
#define NOT_LAST_PACKET 0x0000

#define DATA_PAYLOAD 1024


#include <stdint.h>
#include <vector>
#include "map"

using namespace std;

struct __attribute__((__packed__)) Control_Header {
    uint32_t dest_ip;
    uint8_t control_code;
    uint8_t response_time;
    uint16_t payload_len;
};

struct __attribute__((__packed__)) Control_Response_Header {
    uint32_t controller_ip;
    uint8_t control_code;
    uint8_t response_code;
    uint16_t payload_len;
};

struct __attribute__((__packed__)) routing_packet {
    uint32_t ip;
    uint16_t router_port;
    uint16_t data_port;
    uint16_t padding = 0x00;
    uint16_t router_id;
    uint16_t cost_from_source;
    routing_packet(){}
    routing_packet(uint32_t a, uint16_t b,uint16_t c,uint16_t d,uint16_t e, uint16_t f):
        ip(a), router_port(b), data_port(c), router_id(e), cost_from_source(f){}


};

struct __attribute__((__packed__)) router {
    uint16_t id;
    uint32_t ip;
    uint16_t router_port;
    uint16_t data_port;
    uint16_t operating;
    router(uint16_t id, uint32_t ip,uint16_t router_port,uint16_t data_port, uint16_t operating):
    id(id), ip(ip),router_port(router_port), data_port(data_port), operating(operating){}


};



char *create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len);

int create_routing_packet(char* buffer, uint16_t number, uint16_t source_port, uint32_t source_ip, vector<routing_packet*> &dv);

void extract_routing_packet(uint16_t &number, uint16_t &source_port, uint32_t &source_ip, uint16_t &source_id, vector<routing_packet*> &distant_payload, char* payload);

int create_ROUTING_reponse_payload(char* buffer, map<uint16_t , uint16_t > &DV, map<uint16_t, uint16_t > &next_hops);

int create_tcp_pkt(char* new_pkt, uint32_t destination_ip, uint8_t transfer_id, uint8_t ttl, uint16_t seq_number, uint32_t fin_seq, char* payload);

int modify_tcp_pkt(char* new_pkt, char* pkt, uint32_t &ip);

void route_to_next_hop(char *tcp_pkt, map<uint16_t, uint16_t> &next_hops, map<uint16_t , router*> &all_nodes);

#endif //PROJECT_CONTROL_HEADER_H
