//
// Created by bilin on 11/29/18.
//


#include "../include/connection_manager.h"
#include "../include/global.h"
#include "../include/control_handler.h"
#include "../include/data_handler.h"

#include <vector>
#include <iostream>
#include <cstring>

using namespace std;

int control_socket;

struct timeval tv;
struct timeval next_event_time;



bool first_time;
fd_set master_list;
fd_set watch_list;
int head_fd;


void main_loop();





void udp_broad_cast_hello(uint16_t local_port , map<uint16_t , Router > &neighbors){
    cout << "send dv to neighbors!" << endl;
    char *dv  = "HELLO ";
    for (auto a : neighbors) {

        cout << "sending hello to neighbor " << a.first << " whose ip is" << a.second.ip << " and whose router port is " << a.second.route_port << endl;

        string destAddress = to_string(a.second.ip);             // First arg:  destination address
        unsigned short destPort = a.second.route_port;  // Second arg: destination port


        char* sendString = "GGWP";

        try {
            UDPSocket sock;

            sock.sendTo(sendString, strlen(sendString), destAddress, destPort);

        } catch (SocketException &e) {
            cerr << e.what() << endl;
            exit(1);
        }




    }




}



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

            cout << "Initialized..." << endl;

            for(auto a : immediate_neighbors){
                cout << a.first << "\t" << a.second << endl;
            }

            selret = select(head_fd + 1, &watch_list, NULL, NULL, &tv);
        }


        if (selret < 0) ERROR("select failed.");



        if(selret == 0){
            cout << "Timeout " << endl;
//            udp_broad_cast_hello(my_router_port, )
            break;
        }




        for (sock_index = 0; sock_index <= head_fd; sock_index += 1) {


            if (FD_ISSET(sock_index, &watch_list)) {


                /* control_socket */
                if (sock_index == control_socket) {// TCP, need to create a new connection
                    cout << "curr sock is control socket "<< sock_index << endl;


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
                    cout << "curr sock is data socket " << sock_index << endl;


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