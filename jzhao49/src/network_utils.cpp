//
// Created by bilin on 11/29/18.
//

#include "network_utils.h"
#include <vector>;
#include<bits/stdc++.h>
#include <global.h>
#include <arpa/inet.h>


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






void initialize_dv(vector<vector<int>> &DV, int router_id, int total_routers){

    DV.resize(total_routers, vector<int>(total_routers,INT_MAX));


    for(int i = 0; i < DV.size(); i++){
        for(int j = 0; j < DV[0].size(); j++){
            if(i == router_id - 1 || j == router_id-1) DV[i][j] = 0;

        }
    }

    return;

}



void display_routing_table(vector<vector<int>> &DV){
    for(int i = 0; i < DV.size(); i++){
        for(int j = 0; j < DV[0].size(); j++){
            int value = DV[i][j] == INT_MAX ? 99 : DV[i][j];
            cout << value << "\t";
        }
        cout << endl;
    }

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