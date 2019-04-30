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
#include <control_header_lib.h>


using namespace std;

int control_socket;

struct timeval tv;
struct timeval next_send_time;
struct timeval next_event_time;
const int MAXRCVSTRING = 4096; // Longest string to receive



map <uint16_t, __time_t > next_expected_time;




bool first_time;
fd_set master_list;
fd_set watch_list;
int head_fd;


void main_loop();






void udp_broad_cast_DV(uint16_t local_port , uint16_t total_numbers, map<uint16_t ,uint16_t > &DV, map<uint16_t , routing_packet > &neighbors){

    vector<routing_packet*> tmp;

    for(int i = 1; i <= total_numbers; i++){

        routing_packet *p;
        if(neighbors.find(i) != neighbors.end()){
            p = new routing_packet(neighbors[i].ip, neighbors[i].router_port,neighbors[i].data_port,0x00, neighbors[i].router_id, DV[i]);
        }

        else{
            p = new routing_packet(0x00, 0x00, 0x00, 0x00, i, DV[i]);
        }

        tmp.push_back(p);

    }




    char buff[256];

    int bytes = create_routing_packet(buff, router_number, local_port, self.ip, tmp);


    cout << "sending out " << bytes << endl;



    for (auto a : neighbors) {

        string destAddress = to_string(a.second.ip);             // First arg:  destination address

        unsigned short destPort = a.second.router_port;  // Second arg: destination port



        try {
            UDPSocket sock;

            sock.sendTo(buff, bytes, destAddress, destPort);

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


    struct timeval curr_time;



    next_event_time.tv_sec = 0; // init





    while (true) {

        watch_list = master_list;
        if (first_time) {
            selret = select(head_fd + 1, &watch_list, NULL, NULL, NULL);

        }
        else {




            selret = select(head_fd + 1, &watch_list, NULL, NULL, &tv);

        }


        if (selret < 0) ERROR("select failed.");



        gettimeofday(&curr_time, NULL);



        if(selret == 0){

            struct timeval diff = diff_tv(curr_time, next_send_time);

            if (diff.tv_sec == 0) {

                next_send_time.tv_sec = curr_time.tv_sec + time_period;

//                udp_broad_cast_hello(my_router_port, immediate_neighbors);
                udp_broad_cast_DV(self.router_port,router_number, DV,neighbors);

                for(auto a : neighbors){

                    uint32_t ip = a.second.ip;
                    uint16_t data_port = a.second.data_port;
                    string v = print_ip(ip);
                    const char *cstr = v.c_str();

                    tcp_send_hello_to_neighbor(cstr, data_port);

                }

                cout << endl;

                tv = diff_tv(next_send_time, curr_time);
            }
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



                    gettimeofday(&curr_time, NULL);


                    for(auto a : next_expected_time){
                        if(a.second < curr_time.tv_sec){
                            cout << a.first << " is CRASHED !!!" << endl;
                        }


                    }


                    uint16_t number;
                    uint16_t source_port;
                    uint32_t source_ip;
                    uint16_t source_id;

                    vector<routing_packet*> distant_payload;


                        char recvString[MAXRCVSTRING + 1]; // Buffer for echo string + \0
                        string sourceAddress;              // Address of datagram source
                        unsigned short sourcePort;         // Port of datagram source


                        int bytesRcvd = udp_recvFrom(sock_index,recvString, MAXRCVSTRING, sourceAddress,sourcePort);


                        cout << "---> received bytes: " << bytesRcvd << endl;


                        char*buf = recvString;

                    extract_routing_packet(number, source_port,  source_ip, source_id, distant_payload, buf);


                    gettimeofday(&curr_time, NULL);


                    cout << "***************received routing packet from  " << source_id << " at " << curr_time.tv_sec  <<  endl;

                    next_expected_time[source_id] = curr_time.tv_sec + 3 * time_period;




                    if(source_port == self.router_port) continue;

                    update_dv(DV, next_hops, all_nodes, source_id, distant_payload);

                    display_DV(DV, next_hops);
//
//                    display_all_nodes(all_nodes);




                }

                    /* data_socket */
                else if (sock_index == data_socket) {// TCP, need to create link
                    cout << "curr sock is data socket " << sock_index << endl;

                    listen_on_tcp_server(sock_index);


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