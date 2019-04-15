//
// Created by bilin on 11/29/18.
//

#include "../include/utils.h"

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

struct timeval add_tv(struct timeval tv1, struct timeval tv2) {
    struct timeval tv_add;
    tv_add.tv_sec = tv1.tv_sec + tv2.tv_sec;
    tv_add.tv_usec = tv1.tv_usec + tv2.tv_usec;
    if (tv_add.tv_usec > 1000000) {
        tv_add.tv_sec++;
        tv_add.tv_usec -= 1000000;
    }
    return tv_add;
}

ssize_t send_udp(int sock_index, char *buffer, ssize_t nbytes, uint32_t ip, uint16_t port) {
    ssize_t bytes = 0;
    struct sockaddr_in remote_router_addr;
    socklen_t addrlen = sizeof(remote_router_addr);

    bzero(&remote_router_addr, sizeof(remote_router_addr));

    remote_router_addr.sin_family = AF_INET;
    remote_router_addr.sin_addr.s_addr = htonl(ip);
    remote_router_addr.sin_port = htons(port);

    bytes = sendto(sock_index, buffer, (size_t) nbytes, 0, (struct sockaddr *) &remote_router_addr,
                   addrlen);

    if (bytes == 0) return -1;

    return bytes;
}