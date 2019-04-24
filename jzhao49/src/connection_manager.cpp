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
#include <network_utils.h>

using namespace std;

int control_socket;

struct timeval tv;
struct timeval next_event_time;
const int MAXRCVSTRING = 4096; // Longest string to receive




bool first_time;
fd_set master_list;
fd_set watch_list;
int head_fd;


void main_loop();




void print_ip(unsigned int ip)
{
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);
}


void udp_broad_cast_hello(uint16_t local_port , vector<pair<uint16_t, uint32_t >> &neighbors){
    cout << "send hello to neighbors!" << endl;

    for (auto a : neighbors) {

        cout << "sending hello to neighbor " << a.second << " " << a.first << endl;

        string destAddress = to_string(a.second);             // First arg:  destination address

        print_ip(a.second);
        unsigned short destPort = a.first;  // Second arg: destination port


        string tmp = "GGWP from " + to_string(local_port);

        char sendString[tmp.size() + 1];
        strcpy(sendString, tmp.c_str());	// or pass &s[0]


//        char* sendString = "GGWP" + to_string(local_port);

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

            selret = select(head_fd + 1, &watch_list, NULL, NULL, &tv);
        }


        if (selret < 0) ERROR("select failed.");



        if(selret == 0){
            cout << "Timeout " << endl;
            udp_broad_cast_hello(my_router_port, immediate_neighbors);
        }




        for (sock_index = 0; sock_index <= head_fd; sock_index += 1) {
            cout << sock_index << endl;


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

                        char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
                        string sourceAddress;              // Address of datagram source
                        unsigned short sourcePort;         // Port of datagram source


                        int bytesRcvd = udp_recvFrom(sock_index,recvString, MAXRCVSTRING, sourceAddress,sourcePort);
                        recvString[bytesRcvd] = '\0';  // Terminate string
                        cout << "Received " << recvString << " from " << sourceAddress << ": "<< sourcePort << endl;



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