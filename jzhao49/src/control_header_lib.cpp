//
// Created by bilin on 11/28/18.
//
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <vector>
#include <iostream>
#include "control_header_lib.h"
#include "../include/global.h"
#include <map>
using namespace std;


char *create_response_header(int sock_index, uint8_t control_code, uint8_t response_code, uint16_t payload_len) {
    char *buffer;

    struct Control_Response_Header *control_response_header;
    struct sockaddr_in addr;
    socklen_t addr_size;

    buffer = (char *) malloc(sizeof(char) * CONTROL_HEADER_SIZE);

    control_response_header = (struct Control_Response_Header *) buffer;
    addr_size = sizeof(struct sockaddr_in);
    getpeername(sock_index, (struct sockaddr *) &addr, &addr_size);

    memcpy(&(control_response_header->controller_ip), &(addr.sin_addr), sizeof(struct in_addr));
    control_response_header->control_code = control_code;
    control_response_header->response_code = response_code;
    control_response_header->payload_len = htons(payload_len);

    return buffer;
}


int create_routing_packet(char* buffer, uint16_t number, uint16_t source_port, uint32_t source_ip, vector<routing_packet*> &dv){
    int routing_packet_size = 8 + 12 * number;
    unsigned int byte = 0;

    memcpy(buffer + byte, &number, sizeof(number)); byte+=2;
    memcpy(buffer + byte ,&source_port,sizeof(source_port)); byte +=2;
    memcpy(buffer + byte, &source_ip, sizeof(source_ip)); byte += 4;


    for(routing_packet* packet : dv){

        uint32_t modified_ip = ntohl(packet->ip);
        uint16_t router_port = ntohs(packet->router_port);
        uint16_t p = ntohs(packet->padding);
        uint16_t id = ntohs(packet->router_id);
        uint16_t cost = ntohs(packet->cost_from_source);

        memcpy(buffer+byte,     &modified_ip,     sizeof(modified_ip)); byte+=4;
        memcpy(buffer+byte,     &router_port,     sizeof(router_port));byte+=2;
        memcpy(buffer+byte,     &p,               sizeof(p));byte+=2;
        memcpy(buffer+byte,     &id,              sizeof(id));byte+=2;
        memcpy(buffer+byte,      &cost,           sizeof(cost));byte+=2;
    }

    return byte;


}



int create_ROUTING_reponse_payload(char* buffer, map<uint16_t , uint16_t > &DV, map<uint16_t, uint16_t > &next_hops){
    unsigned int byte = 0;

//    for(auto a : next_hops)
    for(std :: map<uint16_t ,uint16_t >::iterator a = next_hops.begin(); a != next_hops.end(); a++)
    {

        uint16_t destination = a->first;
        uint16_t padding = 0x00;
        uint16_t hop_id = a->second;
        uint16_t cost = DV[a->first];

        memcpy(buffer+byte, &destination, sizeof(destination)); byte+=2;
        memcpy(buffer+byte, &padding, sizeof(padding));         byte+=2;
        memcpy(buffer+byte, &hop_id, sizeof(hop_id));           byte+=2;
        memcpy(buffer+byte, &cost, sizeof(cost));               byte+=2;

    }


return byte;

}





void extract_routing_packet(uint16_t &number, uint16_t &source_port, uint32_t &source_ip, uint16_t &source_id, vector<routing_packet*> &distant_payload, char* payload){


    unsigned int byte = 0;
    memcpy(&number, payload + byte, sizeof(number)); byte+=2;
    memcpy(&source_port, payload + byte, sizeof(source_port)); byte+=2;
    memcpy(&source_ip, payload + byte, sizeof(source_ip)); byte+=4;




    for(int i = 0; i < number; i++){
        uint32_t ip;
        uint16_t router_port;
        uint16_t padding;
        uint16_t id;
        uint16_t cost;
        memcpy(&ip,          payload+byte,sizeof(ip));         byte+=4;
        memcpy(&router_port, payload+byte,sizeof(router_port));byte+=2;
        memcpy(&padding,     payload+byte,sizeof(padding));    byte+=2;
        memcpy(&id,          payload+byte,sizeof(id));         byte+=2;
        memcpy(&cost,        payload+byte,sizeof(cost));       byte+=2;


        if(htons(id) == 0) break;



        // represent a dummy data port here.
        routing_packet *p = new routing_packet(htonl(ip), htons(router_port), 0, 0x00, htons(id),htons(cost));

        distant_payload.push_back(p);

        if(htons(cost) == 0) source_id = htons(id);


        if(MUTE == 0) cout << "--------------------------hear from " << source_id << " which costs " << htons(cost) << " to go to " << htons(id) << " " << ip << endl;

    }



}



int create_tcp_pkt(char* new_pkt, uint32_t destination_ip, uint8_t transfer_id, uint8_t ttl, uint16_t seq_number, uint32_t fin_seq, char* payload){


    bzero(new_pkt, sizeof(new_pkt));

    int byte = 0;

    destination_ip = htonl(destination_ip);
    seq_number = htons(seq_number);


    fin_seq = htonl(fin_seq);



    memcpy(new_pkt + byte, &destination_ip, sizeof(destination_ip)); byte += 4;
    memcpy(new_pkt + byte, &transfer_id, sizeof(uint8_t)); byte += 1;
    memcpy(new_pkt + byte, &ttl, sizeof(uint8_t)); byte += 1;
    memcpy(new_pkt + byte, &seq_number, sizeof(uint16_t)); byte += 2;
    memcpy(new_pkt + byte, &fin_seq, sizeof(uint32_t)); byte += 4;
    memcpy(new_pkt + byte, payload, strlen(payload)); byte += strlen(payload);




    return byte;
}



int modify_tcp_pkt(char* new_pkt, char* pkt, uint32_t &ip, uint8_t &ttl_){


    bzero(new_pkt, sizeof(new_pkt));

    uint32_t destination_ip;
    uint8_t transfer_id;
    uint8_t ttl;
    uint16_t seq_number;


    uint32_t fin_seq;


    int byte = 0;

    memcpy(&destination_ip, pkt + byte, sizeof(destination_ip)); byte += 4;

    destination_ip = ntohl(destination_ip);

    ip = destination_ip;

    memcpy(&transfer_id, pkt + byte, sizeof(uint8_t)); byte += 1;

    memcpy(&ttl, pkt + byte, sizeof(uint8_t)); byte += 1;

    ttl = ttl-1;

    ttl_ = ttl;

    memcpy(&seq_number, pkt + byte, sizeof(uint16_t)); byte += 2;

    seq_number = ntohs(seq_number);



    memcpy(&fin_seq, pkt + byte, sizeof(uint32_t)); byte += 4;

    fin_seq = ntohl(fin_seq);


    char* payload = (char*) malloc(1024);

    memcpy(payload, pkt+byte, sizeof(char) * 1024);

    byte = create_tcp_pkt(new_pkt, destination_ip, transfer_id, ttl,  seq_number, fin_seq,  payload);

    return byte;



}


int copy_pkt(char* new_pkt, char* pkt, uint8_t &ttl, uint16_t &seq){
    bzero(new_pkt, sizeof(new_pkt));

    uint32_t destination_ip;
    uint8_t transfer_id;
    uint32_t fin_seq;


    int byte = 0;

    memcpy(&destination_ip, pkt + byte, sizeof(destination_ip)); byte += 4;

    destination_ip = ntohl(destination_ip);

    memcpy(&transfer_id, pkt + byte, sizeof(uint8_t)); byte += 1;

    memcpy(&ttl, pkt + byte, sizeof(uint8_t)); byte += 1;

    uint16_t  tmp;



    memcpy(&tmp, pkt + byte, sizeof(uint16_t)); byte += 2;

    seq = ntohs(tmp);



    memcpy(&fin_seq, pkt + byte, sizeof(uint32_t)); byte += 4;

    fin_seq = ntohl(fin_seq);


    char* payload = (char*) malloc(1024);

    memcpy(payload, pkt+byte, sizeof(char) * 1024);





    byte = create_tcp_pkt(new_pkt, destination_ip, transfer_id, ttl,  seq, fin_seq,  payload);


    return byte;


}


int extract_tcp_pkt(char* buffer, uint32_t &ip, uint32_t &fin, uint8_t &id, uint8_t &ttl, uint16_t &seq, char* recv){

    bzero(recv, 1024);

    memcpy(&ip, buffer, sizeof(uint32_t));
    memcpy(&id, buffer+4, sizeof(uint8_t));
    memcpy(&ttl, buffer+5, sizeof(uint8_t));
    memcpy(&seq, buffer+6, sizeof(uint16_t));
    memcpy(&fin, buffer + 8, sizeof(uint16_t));
    memcpy(recv, buffer + 12, sizeof(char) * 1024);


    seq = ntohs(seq);
    fin = ntohl(fin);
    ip = ntohl(ip);


}