

#ifndef PROJECT_GLOBAL_H
#define PROJECT_GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>


#define ERROR(err_msg) {perror(err_msg); exit(EXIT_FAILURE);}

#define AUTHOR 0x00
#define INIT 0x01
#define ROUTING_TABLE 0x02
#define UPDATE 0x03
#define CRASH 0x04
#define SENDFILE 0x05
#define SENDFILE_STATS 0x06
#define LAST_DATA_PACKET 0x07
#define PENULTIMATE_DATA_PACKET 0x08

#define INF 65535

#define MUTE 0
#define LAST_FIN 2147483648




#endif //PROJECT_GLOBAL_H
