


#include <stdint.h>
#include <sys/select.h>
#include <time.h>
#include "PracticalSocket.h"
#include <map>
#include <vector>

extern int control_socket, router_socket, data_socket;
extern vector<pair<uint16_t, uint16_t >> immediate_neighbors;





extern uint32_t my_ip;
extern uint16_t my_router_port;
extern uint16_t my_data_port;
extern uint16_t my_id;
extern uint16_t routers_number;
extern uint16_t time_period;
extern int head_fd;

void run(uint16_t control_port);

extern fd_set master_list;
extern struct timeval tv;
extern struct timeval next_send_time;
extern struct timeval next_event_time;
extern bool first_time;


struct __attribute__((__packed__)) Route_Content {
    uint32_t router_ip;
    uint16_t router_port;
    uint16_t padding;
    uint16_t router_id;
    uint16_t link_cost;

};

