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

#include "Utility.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

#define MAX_BUFFER 1024
#define SERVER_PORT 8080

#endif