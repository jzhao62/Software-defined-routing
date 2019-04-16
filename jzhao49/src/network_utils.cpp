//
// Created by bilin on 11/29/18.
//

#include "network_utils.h"

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




