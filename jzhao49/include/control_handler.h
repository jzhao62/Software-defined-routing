//
// Created by bilin on 11/29/18.
//



#define AUTHOR_STATEMENT "I, Jingyi, have read and understood the course academic integrity policy."

#include <stdint.h>
#include <vector>
#include <sys/time.h>
#include <map>
using namespace std;

struct Remote_DV {
    uint16_t dest_id;
    uint16_t cost;
};

struct Router {
    uint16_t id;
    uint16_t route_port;
    uint16_t data_port;
    uint16_t c;
    uint32_t ip;
//    uint16_t total_cost;
//    uint16_t next_hop_id;
    std::vector<Remote_DV> remote_dv;
};

struct Timeout {
    int router_id;
    bool is_connected;
    struct timeval expired_time;
};

struct Route_Neighbor {
    uint16_t router_id;
    int socket;
    uint32_t ip;
    uint16_t port;
};

struct Transfer_File {
    uint8_t transfer_id;
    uint8_t ttl;
    std::vector<uint16_t> sequence;
};

extern std::vector<Router> table;
extern std::vector<Timeout> routers_timeout;
extern std::vector<Transfer_File> transfer_files;

extern char *last_packet;
extern char *penultimate_packet;
extern char *file_buffer;
extern int datagram_count;


int create_control_sock(uint16_t CONTROL_PORT);

int create_route_sock(uint16_t router_port);

int create_data_sock(uint16_t data_sock);




int new_control_conn(int sock_index);

void remove_control_conn(int sock_index);

int new_route_conn(uint32_t remote_ip, uint16_t remote_port, uint16_t remote_id);

void remove_route_conn(uint16_t router_id);

bool isControl(int sock_index);

bool control_recv_hook(int sock_index);

void author(int sock_index);

void initialize(int sock_index, char* payload);


void udp_broad_cast_hello(uint16_t local_port , map<uint16_t , Router > &neighbors);
