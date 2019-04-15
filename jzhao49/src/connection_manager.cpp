//
// Created by bilin on 11/29/18.
//


#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/data_handler.h"
#include "../include/utils.h"

#include <vector>
#include <iostream>

using namespace std;

int control_socket;

struct timeval tv;
struct timeval next_send_time;
struct timeval next_event_time;
bool first_time;

void main_loop();

fd_set master_list;
fd_set watch_list;
int head_fd;


void run(uint16_t control_port) {
    control_socket = create_control_sock(control_port); // create first socket, bind to control port to listen

    //router_socket and data_socket will be initialized after INIT from controller

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the control socket */
    FD_SET(control_socket, &master_list);
    head_fd = control_socket; // head_fd set to maximum, it can be changed anytime a new socket is created

    main_loop(); // main function
}


void main_loop() {
    int selret;
    int sock_index;
    int fdaccept;
    first_time = true;

    next_event_time.tv_sec = 0; // init

    struct timeval cur;
    struct timeval cur_no_timeout;

    while (true) {
        gettimeofday(&cur, NULL);
        cout << "[starting loop] current time: " << cur.tv_sec << endl;

        watch_list = master_list;
        if (first_time) { // if routing table is not initialized, we can only deal with command from controller
            selret = select(head_fd + 1, &watch_list, NULL, NULL, NULL);
//            cout << "time peroid before init: " << time_period << endl;
            cout << "not received init yet!" << endl;
        } else {
            /**
             * now we have routing table, we wait for "tv" time to trigger the next event.
             * it can be either send dv or delete neighbor and update routing table(if we didn't receive dv from
             * it for 3T time)
             * */
//            cout << "time period after init: " << time_period << endl;
            cout << "waiting for tv=" << tv.tv_sec << "." << tv.tv_usec / 100000 << "s..." << endl;
            selret = select(head_fd + 1, &watch_list, NULL, NULL, &tv);
        }

        gettimeofday(&cur, NULL);
        cout << "[after select] current time: " << cur.tv_sec << endl;

        if (selret < 0) ERROR("select failed.");


        if (selret == 0) { // timeout, send or disconnect
            cout << "timeout!" << endl;

            /* get current time*/
            gettimeofday(&cur, NULL);
            cout << "current time: " << cur.tv_sec << endl;

            struct timeval diff; // it store the difference of 2 tv, will be used many times

            diff = diff_tv(cur, next_send_time); // get (cur-send_time)
            if (diff.tv_sec >= 0 || diff.tv_usec >= 0) { // if current time reach the next expected sending time
                /* update next expected sending time to (cur+T) */
                next_send_time.tv_sec = cur.tv_sec + time_period;
                next_send_time.tv_usec = cur.tv_usec;

                cout << "next sending time: " << next_send_time.tv_sec << "s" << endl;

                send_dv(); // send dv to neighbors

                tv = diff_tv(next_send_time, cur); // waiting time is set to T (temporally)
                cout << "set tv to " << tv.tv_sec << "." << tv.tv_usec / 100000 << "s" << endl;
            }
            // don't forget update tv

            // next, router expired setting
            for (int i = 0; i < routers_timeout.size(); i++) { // go through all routers in the network
                if (!routers_timeout[i].is_connected) { // ignore disconnected routers
                    continue;
                }
                cout << "expired time of router " << routers_timeout[i].router_id << " is "
                     << routers_timeout[i].expired_time.tv_sec << endl;
                diff = diff_tv(cur, routers_timeout[i].expired_time);
                if (diff.tv_sec >= 0 || (diff.tv_sec == 0 && diff.tv_usec >= 0)) { // this router timeout
                    /* first set this router unreachable, then set the cost to this router to infinity */
                    routers_timeout[i].is_connected = false;

                    cout << "router: " << routers_timeout[i].router_id << " timeout!" << endl;

                    for (int j = 0; j < table.size(); j++) { // find this router in routing table
                        if (table[j].dest_id == routers_timeout[i].router_id) {
                            /* sent cost to infinity, set next hop to infinity means it's unreachable */
                            table[j].dest_cost = INF;
                            table[j].next_hop_id = INF;
                            break;
                        }
                    }
                    /* we can only receive dv from neighbors, so this router must be my neighbor,
                     * now remove it from my neighbors list */
                    remove_route_conn(routers_timeout[i].router_id);
                } else { // this neighbor does't timeout
                    /* waiting time should be min(T, expire_time - cur) */

                    diff = diff_tv(tv, diff_tv(routers_timeout[i].expired_time, cur));
                    if (diff.tv_sec >= 0 || (diff.tv_sec == 0 && diff.tv_usec >= 0)) {
                        tv = diff_tv(routers_timeout[i].expired_time, cur);
                        cout << "set tv to " << tv.tv_sec << "." << tv.tv_usec / 100000 << "s" << endl;
                    }
                }
            }
            next_event_time = add_tv(cur, tv);
            cout << "[timeout] next event time is: " << next_event_time.tv_sec << endl;
            continue;
        }
        /* Loop through file descriptors to check which ones are ready */
        for (sock_index = 0; sock_index <= head_fd; sock_index += 1) {

            if (FD_ISSET(sock_index, &watch_list)) {

                /* control_socket */
                if (sock_index == control_socket) {// TCP, need to create a new connection
                    cout << "[control connection]" << endl;
                    fdaccept = new_control_conn(sock_index); // create a tcp socket to handle controller commands

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if (fdaccept > head_fd) head_fd = fdaccept;
                }

                    /* router_socket */
                else if (sock_index == router_socket) {
                    cout << "[routing packet]" << endl;
                    /* receive dv from neighbors, should update routing table and waiting time(if needed) */
                    update_routing_table(sock_index);
                }

                    /* data_socket */
                else if (sock_index == data_socket) {// TCP, need to create link
                    cout << "[data connection]" << endl;
                    fdaccept = new_data_conn(sock_index);

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if (fdaccept > head_fd) head_fd = fdaccept;
                }

                    /* Existing connection */
                else {
                    // sockfd is either link to controller(control message), or link to another router(switch datagram)
                    if (isControl(sock_index)) {
                        cout << "[control command]" << endl;
                        if (!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                    } else if (isData(sock_index)) {
                        cout << "[datagram]" << endl;
                        // when switch datagram, first check if connection exist, if not, connect() first, then send()
                        if (!data_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);

                    } else {
                        ERROR("Unknown socket index");
                    }
                }

                if (next_event_time.tv_sec > 0) {
                    gettimeofday(&cur_no_timeout, NULL);
                    struct timeval tmp_diff;
                    tmp_diff = diff_tv(next_event_time, cur_no_timeout);
                    if (tmp_diff.tv_sec >= 0 || (tmp_diff.tv_sec == 0 && tmp_diff.tv_usec >= 0)) {
                        tv = tmp_diff;
                    } else {
                        tv.tv_sec = 0;
                        tv.tv_usec = 0;
                    }
                    cout << "[no timeout] current time is: " << cur_no_timeout.tv_sec << endl;
                    cout << "[other event] next event time is: " << next_event_time.tv_sec << endl;
                    cout << "tv now is: " << tv.tv_sec << "." << tv.tv_usec / 100000 << "s" << endl;
                }
            }

        }
    }
}