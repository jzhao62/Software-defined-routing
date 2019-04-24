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


using namespace std;

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes);

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes);

struct timeval diff_tv(struct timeval tv1, struct timeval tv2);

struct timeval add_tv(struct timeval tv1, struct timeval tv2);

ssize_t send_udp(int sock_index, char *buffer, ssize_t nbytes, uint32_t ip, uint16_t port);


void initialize_dv(vector<vector<int>> &DV, int id, int total_routers);

void display_routing_table(vector<vector<int>> &DV);

int udp_recvFrom(int sock, void *buffer, int bufferLen, string &sourceAddress,unsigned short &sourcePort);


