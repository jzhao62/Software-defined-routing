

#include "network_utils.h"
#include <vector>;
#include<bits/stdc++.h>
#include <global.h>
#include <arpa/inet.h>
#include <control_header_lib.h>
#include "PracticalSocket.h"
#include "sys/socket.h"
#include <netinet/in.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>


using namespace std;

ssize_t recvALL(int sock_index, char *buffer, ssize_t nbytes) {
    ssize_t bytes = 0;
    bytes = recv(sock_index, buffer, nbytes, 0);

    if (bytes == 0) return -1;
    while (bytes != nbytes)
        bytes += recv(sock_index, buffer + bytes, nbytes - bytes, 0);

    return bytes;
}

ssize_t sendALL(int sock_index, char *buffer, ssize_t nbytes) {
    ssize_t bytes = 0;
    bytes = send(sock_index, buffer, nbytes, 0);

    if (bytes == 0) return -1;
    while (bytes != nbytes)
        bytes += send(sock_index, buffer + bytes, nbytes - bytes, 0);

    return bytes;
}






void initialize_dv(map<uint16_t, uint16_t > &DV, map<uint16_t, uint16_t > &next_hops, int router_id, int total_routers){


    for(int i = 1; i <= total_routers; i++){
        if(router_id == i) {
            DV[i] = 0;
            next_hops[i] = i;
        }
        else DV[i] = UINT_MAX;
    }

}



void display_DV(map<uint16_t ,uint16_t > &DV, map<uint16_t ,uint16_t > &next_hops){

    cout << "=================" << endl;
    for(auto a : DV){
        cout << " from current to " << a.first << " costs " << a.second << endl;
    }
    cout << "=================" << endl;

    cout << "=================" << endl;
    for(auto a : next_hops){
        cout << " from current to " << a.first << " should go from " << a.second << endl;
    }
    cout << "=================" << endl;


}



int udp_recvFrom(int sock, void *buffer, int bufferLen, string &sourceAddress,unsigned short &sourcePort) {
    sockaddr_in clntAddr;
    socklen_t addrLen = sizeof(clntAddr);
    int rtn;
    if ((rtn = recvfrom(sock,  buffer, bufferLen, 0,(sockaddr *) &clntAddr, (socklen_t *) &addrLen)) < 0) {
        ERROR("RECEIVED UDP ERROW")
    }

    sourceAddress = inet_ntoa(clntAddr.sin_addr);
    sourcePort = ntohs(clntAddr.sin_port);


    return rtn;
}



struct timeval diff_tv(struct timeval tv1, struct timeval tv2) {

    struct timeval tv_diff;
    tv_diff.tv_sec = tv1.tv_sec - tv2.tv_sec;
    tv_diff.tv_usec = tv1.tv_usec - tv2.tv_usec;


    if (tv_diff.tv_usec < 0) {

        tv_diff.tv_sec--;
        tv_diff.tv_usec += 1000000;
    }


    return tv_diff;
}




string print_ip(unsigned int ip)
{
    cout << ip << endl;

    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);


    int v1 = (int) bytes[3];
    int v2 = (int) bytes[2];
    int v3 = (int) bytes[1];
    int v4 = (int) bytes[0];


    string v = to_string(v1) + '.' + to_string(v2) + '.' +to_string(v3) + '.' + to_string(v4);

    return v;
}






void update_dv(map<uint16_t ,uint16_t > &DV,map<uint16_t ,uint16_t > &next_hops, map<uint16_t , router*> &all_nodes, uint16_t source_id, vector<routing_packet*> &received_pkts){

    map<uint16_t ,uint16_t > received_dv;

    for(auto a : received_pkts){
        if(all_nodes.find(a->router_id) == all_nodes.end() && a->router_port != 0){
            cout << a->router_id << " " << a->router_port << endl;
            all_nodes[a->router_id] = new router(a->router_id, a->ip, a->router_port, a->data_port, 1);
        }

        received_dv[a->router_id] = a->cost_from_source;

    }

    uint16_t cost_to_source = DV[source_id];


    cout << "cost to " << source_id << " is " << cost_to_source << endl;

    for(auto &itr : DV){

        uint16_t destination = itr.first;
        if(itr.second < cost_to_source + received_dv[destination]) continue;
        if(cost_to_source + received_dv[destination] == 0) continue;
        if(destination == source_id) continue;

        itr.second = cost_to_source + received_dv[destination];
        cout << "updated to " << "[" << itr.first << "]" << " cost " << cost_to_source << " + " << received_dv[destination] << endl;
        next_hops[destination] = source_id;


    }



}


void display_all_nodes(map<uint16_t , router*> &all_nodes){
    cout << "=============" << endl;
    for(auto a : all_nodes){
        cout << " " <<  a.first << " -> " << a.second->router_port << " & " << a.second->data_port << endl;
    }
    cout << "=============" << endl;

}



void udp_broad_cast_hello(uint16_t local_port , vector<pair<uint16_t, uint32_t >> &neighbors){
    for (auto a : neighbors) {

        string destAddress = to_string(a.second);             // First arg:  destination address

        print_ip(a.second);
        unsigned short destPort = a.first;  // Second arg: destination port


        string tmp = "GGWP from " + to_string(local_port);


        char sendString[tmp.size() + 1];
        strcpy(sendString, tmp.c_str());	// or pass &s[0]


        try {
            UDPSocket sock;

            cout << "send out bytes " << strlen(sendString) << endl;

            sock.sendTo(sendString, strlen(sendString), destAddress, destPort);


        } catch (SocketException &e) {
            cerr << e.what() << endl;
            exit(1);
        }

    }




}


void tcp_send_hello(uint16_t local_port, uint32_t destination_ip , uint16_t destination_port, map<uint16_t , uint16_t > &next_hops){


        string destAddress = to_string(destination_ip);             // First arg:  destination address

        print_ip(destination_ip);
        unsigned short destPort = destination_port;  // Second arg: destination port


        string tmp = "GGWP from " + to_string(local_port);


        char sendString[tmp.size() + 1];
        strcpy(sendString, tmp.c_str());	// or pass &s[0]


        try {
            TCPSocket sock;

            cout << "send out bytes " << strlen(sendString) << endl;

            sock.send(sendString, strlen(sendString));


        } catch (SocketException &e) {
            cerr << e.what() << endl;
            exit(1);
        }


}


// modify ttl and send pkt to the next hop according to destination
void route_to_next_hop(char *tcp_pkt, map<uint16_t, uint16_t> &next_hops, map<uint16_t , router*> &all_nodes){

    char* new_pkt;

    uint32_t ip;
    uint16_t port;

    int bytes = modify_tcp_pkt(new_pkt, tcp_pkt, port, ip);

    for(auto n : all_nodes){
        if(n.second->ip == ip && n.second->data_port == port){

            TCPSocket sock;

            sock.send(new_pkt, bytes);


        }



    }




}





int listen_on_tcp_server(int sockfd){
    int newsockfd;
    socklen_t clilen;
    char buffer[1025];
    struct sockaddr_in cli_addr;
    int n;

    listen(sockfd,5);

    // The accept() call actually accepts an incoming connection
    clilen = sizeof(cli_addr);



    newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) ERROR("ERROR on accept");

    printf("server: got connection from %s port %d\n",inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));


    string ret = "";




    bzero(buffer,1025);

    n = read(newsockfd,buffer,1025);




    if(n != 0){


            string tmp = buffer;
            tmp = tmp.substr(0,1024);
            cout << "received tcp msgs" << endl;


            ret += tmp;
            cout << ret << endl;
    }



    if (n < 0) ERROR("ERROR reading from socket");


    close(newsockfd);
    return 0;

}


void tcp_send_hello_to_neighbor(const char* server_ip, int portno){

    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[1024];


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)ERROR("ERROR opening socket");
    server = gethostbyname(server_ip);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
    serv_addr.sin_port = htons(portno);

    bzero(buffer,1024);


    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)ERROR("ERROR connecting");


    char* sendstring = "GGWP";

    memcpy(buffer, sendstring, strlen(sendstring));


    n = write(sockfd, buffer, strlen(sendstring));

    cout << "sending out GGWP" << endl;
    if (n < 0) ERROR("ERROR writing to socket");


    close(sockfd);


}








//void post_crash(map<uint16_t ,uint16_t > &DV,map<uint16_t ,uint16_t > &next_hops, map<uint16_t , router*> &all_nodes,int router_id, int total_routers, map<uint16_t , routing_packet > &neighbors){
//
//
//
//    map<uint16_t ,uint16_t > *tmp_dv = &DV;
//
//
//    for(auto n : neighbors){
//
//        int id = int(n.first);
//
//        if(id == router_id) {
//
//        }
//
//
//        DV[id] = cost;
//    }
//
//
//    display_DV(DV, next_hops);
//
//
//
//
//
//
//}
