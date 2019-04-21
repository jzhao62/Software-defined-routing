//
// Created by bilin on 11/29/18.
//


#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/data_handler.h"

#include <vector>
#include <iostream>

using namespace std;

int control_socket;

struct timeval tv;
struct timeval next_event_time;


bool first_time;

void main_loop();

fd_set master_list;
fd_set watch_list;
int head_fd;


void run(uint16_t control_port) {
    control_socket = create_control_sock(control_port);

    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    FD_SET(control_socket, &master_list);
    head_fd = control_socket;

    main_loop(); // main function
}


void main_loop() {
    int selret;
    int sock_index;
    int fdaccept;
    first_time = true;

    next_event_time.tv_sec = 0; // init

    while (true) {

        watch_list = master_list;
        if (first_time) {
            selret = select(head_fd + 1, &watch_list, NULL, NULL, NULL);

        }
        else {
            /**
             * now we have routing table, we wait for "tv" time to trigger the next event.
             * it can be either send dv or delete neighbor and update routing table(if we didn't receive dv from
             * it for 3T time)
             * */
//            cout << "time period after init: " << time_period << endl;
            selret = select(head_fd + 1, &watch_list, NULL, NULL, &tv);
        }


        if (selret < 0) ERROR("select failed.");




        for (sock_index = 0; sock_index <= head_fd; sock_index += 1) {


            if (FD_ISSET(sock_index, &watch_list)) {
                cout << "curr sock "<< sock_index << endl;


                /* control_socket */
                if (sock_index == control_socket) {// TCP, need to create a new connection

                    fdaccept = new_control_conn(sock_index); // create a tcp socket to handle controller commands

                    /* Add to watched socket list */
                    FD_SET(fdaccept, &master_list);
                    if (fdaccept > head_fd) head_fd = fdaccept;
                }

                    /* router_socket */
                else if (sock_index == router_socket) {

                    cout << "routing packet" << endl;

                }

                    /* data_socket */
                else if (sock_index == data_socket) {// TCP, need to create link

                }

                    /* Existing connection */
                else {
                    // sockfd is either link to controller(control message), or link to another router(switch datagram)
                    if (isControl(sock_index)) {
//                        cout << "[control command]" << endl;
                        if (!control_recv_hook(sock_index)) FD_CLR(sock_index, &master_list);
                    }


                    else {
                        ERROR("Unknown socket index");
                    }
                }


            }

        }
    }
}