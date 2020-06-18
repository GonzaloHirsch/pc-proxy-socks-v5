#ifndef __SCTPCLIENT_H__
#define __SCTPCLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include "../tests/Utility.h"
#include "sctpArgs.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

#define MAX_BUFFER 1024
#define MAX_OPT_SIZE 4

typedef enum Options {
    OPT_EXIT = 0x01,
    OPT_LIST_USERS = 0x02,
    OPT_CREATE_USER = 0x03,
    OPT_SHOW_METRICS = 0x04,
    OPT_SHOW_CONFIGS = 0x05
} Options;

#endif