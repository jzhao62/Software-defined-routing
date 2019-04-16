//
// Created by bilin on 11/28/18.
//

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

struct __attribute__((__packed__)) Data_Header {
    uint32_t dest_ip;
    uint8_t transfer_id;
    uint8_t ttl;
    uint16_t seq_num;
    uint32_t fin_padding;
};

struct __attribute__((__packed__)) Route_Header {
    uint16_t num_update;
    uint16_t source_port;
    uint32_t source_ip;
};

struct __attribute__((__packed__)) Init_Payoad {
    uint16_t router_id;
    uint16_t router_port;
    uint16_t data_port;
    uint16_t cost;
    uint32_t router_ip;
};

char *create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len);

char *create_data_header(uint32_t dest_ip, uint8_t transfer_id, uint8_t ttl, uint16_t seq_num, int fin);

#endif //PROJECT_CONTROL_HEADER_H
