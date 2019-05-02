//
// Created by bilin on 11/29/18.
//

#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdint.h>
#include <netinet/in.h>
#include <strings.h>
#include <vector>
#include <string>
#include <map>
#include <control_header_lib.h>

using namespace std;

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes);

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes);


struct timeval add_tv(struct timeval tv1, struct timeval tv2);

ssize_t send_udp(int sock_index, char *buffer, ssize_t nbytes, uint32_t ip, uint16_t port);


int udp_recvFrom(int sock, void *buffer, int bufferLen, string &sourceAddress,unsigned short &sourcePort);


struct timeval diff_tv(struct timeval tv1, struct timeval tv2);

void update_dv(map<uint16_t ,uint16_t > &DV, map<uint16_t ,uint16_t > &next_hops, map<uint16_t , router*> &all_nodes, uint16_t source_id, vector<routing_packet*> &received_pkts);

void initialize_dv(map<uint16_t , uint16_t> &DV, map<uint16_t, uint16_t > &next_hops, int id, int total_routers);

void display_DV(map<uint16_t ,uint16_t > &DV, map<uint16_t ,uint16_t > &next_hops);

void display_all_nodes(map<uint16_t , router*> &all_nodes);

string print_ip(unsigned int ip);



void udp_broad_cast_hello(uint16_t local_port , vector<pair<uint16_t, uint32_t >> &neighbors);


void tcp_send_pkt_to_neighbor(const char *server_ip, int portno, char *buff, int bytes);

void listen_on_tcp_server(char buffer [], int sock_fd);



vector<pair<char*, int>> load_file_contents(const char* filename);