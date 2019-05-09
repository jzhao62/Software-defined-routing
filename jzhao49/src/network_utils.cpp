

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
#include "fstream"

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


//map<uint16_t , uint16_t > old_dv;
map <uint16_t , int> update_times;


void initialize_dv(map<uint16_t, uint16_t > &DV, map<uint16_t, uint16_t > &next_hops, int router_id, int total_routers){


    for(int i = 1; i <= total_routers; i++){
        if(router_id == i) {
            DV[i] = 0;
            next_hops[i] = i;
        }
        else DV[i] = 10000;
    }

}



void display_DV(map<uint16_t ,uint16_t > &DV, map<uint16_t ,uint16_t > &next_hops){

    cout << "===========================" << endl;
//    for(auto a : DV)

    for(std :: map<uint16_t ,uint16_t >::iterator a = DV.begin(); a != DV.end(); a++)
    {
        cout << " from current to " << a->first << " costs " << a->second  << " via " << next_hops[a->first] << endl;
    }
    cout << "-----------------" << endl;

    cout << "-----------------" << endl;
//    for(auto a : next_hops)


//    for(std :: map<uint16_t ,uint16_t >::iterator a = next_hops.begin(); a != next_hops.end(); a++)
//    {
//        cout << " from current to " << a->first << " should go from " << a->second << endl;
//    }
//    cout << "===========================" << endl;


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
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;
//    printf("%d.%d.%d.%d\n", bytes[3], bytes[2], bytes[1], bytes[0]);


    int v1 = (int) bytes[3];
    int v2 = (int) bytes[2];
    int v3 = (int) bytes[1];
    int v4 = (int) bytes[0];

    std::string p1;
    std::string p2;
    std::string p3;
    std::string p4;

    std::stringstream ss;

    ss << v1;
    p1 = ss.str();

    ss.str("");
    ss << v2;
    p2 = ss.str();

    ss.str("");
    ss << v3;
    p3 = ss.str();

    ss.str("");
    ss << v4;
    p4 = ss.str();

    ss.str("");

    string v = p1 + '.' +  p2 + '.' + p3 + '.' + p4;


//    string v = to_string(v1) + '.' + to_string(v2) + '.' +to_string(v3) + '.' + to_string(v4);

    return v;
}

void update_dv(map<uint16_t ,uint16_t > &DV,map<uint16_t ,uint16_t > &next_hops, map<uint16_t , router*> &all_nodes, uint16_t local_id, uint16_t source_id, vector<routing_packet*> &received_pkts){


    map<uint16_t ,uint16_t > received_dv;
    map<uint16_t ,uint16_t > prev_dv = DV;
    map<uint16_t ,uint16_t > prev_hop = next_hops;

//    for(auto a : received_pkts)
//    for(routing_packet* a : received_pkts)

    for(int i = 0; i < received_pkts.size(); i++)
    {
        routing_packet* a = received_pkts[i];
        if(all_nodes.find(a->router_id) == all_nodes.end() || all_nodes[a->router_id]->ip == 0){
            all_nodes[a->router_id] = new router(a->router_id, a->ip, a->router_port, a->data_port, 1);
            cout << "newly added " << a->router_id << " " << a->ip<< endl;
        }

        received_dv[a->router_id] = a->cost_from_source;

    }





    uint16_t cost_to_source = DV[source_id];



//    for(auto &itr : DV)

    for(map<uint16_t ,uint16_t >::iterator itr = DV.begin(); itr != DV.end(); itr++)

    {

        uint16_t destination = itr->first;

        // only works for nodes that are adjacent to the crashed node
        if(all_nodes[destination] != NULL && all_nodes[destination]->operating == 0){
            DV[destination] = INF;
            next_hops[destination] = INF;
            continue;
        }


        if(received_dv[destination] == INF && source_id != local_id){
            if(DV[destination] == INF && next_hops[destination] == INF) continue;
            DV[destination] = INF;
            next_hops[destination] = INF;
            cout << "setting cost to" << destination << " as INF" << endl;
            continue;
        }


        if(itr->second <= cost_to_source + received_dv[destination] && next_hops[destination] != source_id) continue;
        if(cost_to_source + received_dv[destination] == 0) continue;
        if(destination == source_id) continue;


        if(received_dv[destination] != INF){
               DV[itr->first] = cost_to_source + received_dv[destination];
                next_hops[destination] = source_id;
        }

    }


//    for(auto &a : all_nodes)

    for(map<uint16_t ,router* >::iterator a = all_nodes.begin(); a != all_nodes.end(); a++)
    {
        if(a->second && a->second->operating == 0) next_hops[a->first] = INF;
    }




}

void display_all_nodes(map<uint16_t , router*> &all_nodes){
    cout << "=============" << endl;
//    for(auto a : all_nodes)
     for(map<uint16_t ,router* >::iterator a = all_nodes.begin(); a != all_nodes.end(); a++)
    {
        if(a->second == NULL) continue;
        cout << " " <<  a->first << " operating " << a->second->operating << " ip " << a->second->ip << " data port " << a->second->data_port << endl;
    }
    cout << "=============" << endl;

}






// modify ttl and send pkt to the next hop according to destination
void route_to_next_hop(char *tcp_pkt, map<uint16_t, uint16_t> &next_hops, map<uint16_t , router*> &all_nodes){

    char* new_pkt = (char *) malloc(sizeof(char) * (1024 + 12));


    uint32_t destination_ip;

    int destination_id = 0;
    int next_hop_id = 0;
    uint32_t next_hop_ip = 0;
    uint16_t next_hop_data_port = 0;

    uint8_t post_ttl;

    int bytes = modify_tcp_pkt(new_pkt, tcp_pkt, destination_ip, post_ttl);

    if(post_ttl <= 0) {
        cout << "pkts dropped " << endl;
        return;
    }


    cout << bytes << " bytes: Modified pkt " << endl;



//    for(auto a : all_nodes)

        for(map<uint16_t ,router* >::iterator a = all_nodes.begin(); a != all_nodes.end(); a++)
        {
        if(a->second->ip == destination_ip) {
            destination_id = a->first;
            break;
        }

    }




    next_hop_id = next_hops[destination_id];

    next_hop_ip = all_nodes[next_hop_id]->ip;
    next_hop_data_port = all_nodes[next_hop_id]->data_port;



    string v = print_ip(next_hop_ip);
    const char *cstr = v.c_str();


    printf("sending pkt to %d\n" , cstr);
    tcp_send_pkt_to_neighbor(cstr, next_hop_data_port, new_pkt, bytes);
    printf("pkt sent \n");





}





void listen_on_tcp_server(char buffer [], int sockfd){
    int newsockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    int n;

    listen(sockfd,5);

    // The accept() call actually accepts an incoming connection
    clilen = sizeof(cli_addr);



    newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) ERROR("ERROR on accept");

    printf("server: got connection from %s port %d\n",inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));


    string ret = "";




    bzero(buffer,12 + 1025);

    n = read(newsockfd,buffer,12 + 1025);



    printf("read %d bytes \n" , n);


    if (n < 0) ERROR("ERROR reading from socket");


    close(newsockfd);

}


void tcp_send_pkt_to_neighbor(const char *server_ip, int portno, char *buff, int byte){

    int sockfd, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[1036];


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

    bzero(buffer,1036);


    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)ERROR("ERROR connecting");

    memcpy(buffer, buff, byte);

    n = write(sockfd, buffer, byte);

    if (n < 0) ERROR("ERROR writing to socket");


    close(sockfd);


}

std::ifstream::pos_type file_len(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

vector<pair<char*, int> > load_file_contents(const char* filename){

    vector<pair<char*, int> > ret;

    FILE* filehandle = fopen(filename,"r");

    unsigned int filesize = file_len(filename);


    int byte = 0;

    char *chunck;


    for(byte = 0; byte <= filesize ; byte += 1024){

        if(byte + 1024 >= filesize){
            int new_size = filesize - byte;
            chunck = (char*)malloc(sizeof(char)*(new_size));
            int bytesread = fread(chunck, sizeof(char), new_size, filehandle);

            cout << "---------------" << endl;
            cout << chunck << endl;
            cout << "---------------" << endl;

            ret.push_back({chunck, bytesread});
            break;
        }

        chunck = (char*)malloc(sizeof(char)*1024);
        int bytesread = fread(chunck, sizeof(char), 1024, filehandle);

        cout << "---------------" << endl;
        cout << chunck << endl;
        cout << "---------------" << endl;


        ret.push_back({chunck, bytesread});



    }

    printf("---load file pkts complete\n");

    return ret;

//    rewind(filehandle);



}


/**
 *
 * @param DV local DV (to router : cost)
 * @param next_hops  (target : next_hop)
 * @param all_nodes list of active nodes;
 * @param router_id  node that is down
 * @param total_routers
 * @param neighbors
 */


void post_crash(map<uint16_t ,uint16_t > &DV,map<uint16_t ,uint16_t > &next_hops, map<uint16_t , router*> &all_nodes,int current_id, int crashed_id, map<uint16_t , routing_packet > &neighbors){

//    old_dv = DV;

//    cout << "-------pre crash DV" << endl;
//    display_DV(old_dv,next_hops);
//    cout << "-------pre crash DV" << endl;


    all_nodes[crashed_id]->operating = 0;

    map<uint16_t ,uint16_t > *tmp_dv = &DV;


    vector<routing_packet*> payloads;


//    for(auto a : all_nodes)
    for(map<uint16_t ,router* >::iterator a = all_nodes.begin(); a != all_nodes.end(); a++)

    {
        routing_packet *p = new routing_packet(0, 0,0,0,a->first, INF);
        payloads.push_back(p);
    }


    DV[crashed_id] = INF;
    next_hops[crashed_id] = INF;


//    for(auto n : next_hops)

    for(map<uint16_t ,uint16_t >::iterator n = next_hops.begin(); n != next_hops.end(); n++)

    {
        if(n->second == crashed_id) {
            DV[n->first] = INF;
            next_hops[n->first] = INF;
        }
    }




    cout << " POST CRASH  " << endl;
     display_DV(DV, next_hops);

    update_dv(DV,next_hops,all_nodes, current_id, current_id, payloads);





}



void write_datas_to_file(uint8_t id, vector<char*> &received_data){

//    string p1;
//    stringstream ss;
//    ss << unsigned(id);
//    p1 = ss.str();
//
//    string file_name = "file_" + p1;
//
//    cout << file_name << endl;
//
//    ofstream myfile;
//    myfile.open (file_name);
////    for(auto p : received_data)
//
////    for(char* p : received_data)
//
//    for(int i = 0; i < received_data.size(); i++)
//    {
//        char* p = received_data[i];
//        char* recv = (char*) malloc(1024);
//        bzero(recv, 1024);
//        memcpy(recv, p, sizeof(char) * 1024);
//
//      myfile << recv;
//
//    }
//    myfile.close();



}


//void tcp_send_hello(uint16_t local_port, uint32_t destination_ip , uint16_t destination_port, map<uint16_t , uint16_t > &next_hops){
//
//
//        string destAddress = to_string(destination_ip);             // First arg:  destination address
//
//        print_ip(destination_ip);
//        unsigned short destPort = destination_port;  // Second arg: destination port
//
//
//        string tmp = "GGWP from " + to_string(local_port);
//
//
//        char sendString[tmp.size() + 1];
//        strcpy(sendString, tmp.c_str());	// or pass &s[0]
//
//
//        try {
//            TCPSocket sock;
//
//            cout << "send out bytes " << strlen(sendString) << endl;
//
//            sock.send(sendString, strlen(sendString));
//
//
//        } catch (SocketException &e) {
//            cerr << e.what() << endl;
//            exit(1);
//        }
//
//
//}




//bool update_complete(map<uint16_t ,uint16_t > &DV, map<uint16_t ,uint16_t > &prev_dv, map<uint16_t ,uint16_t > &next_hops, map<uint16_t ,uint16_t > &prev_hop){
//    for(map<uint16_t ,uint16_t >::iterator itr = DV.begin(); itr != DV.end(); itr++){
//        uint16_t destination = itr->first;
//        if(DV[destination] != prev_dv[destination] || next_hops[destination] != prev_hop[destination]){
//            return false;
//        }
//
//    }
//
//    return true;
//
//}