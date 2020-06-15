#ifndef __SOCKSV5_H__
#define __SOCKSV5_H__

#include "io_utils/buffer.h"
#include "parsers/hello_parser.h"
#include "parsers/socks_5_addr_parser.h"
#include "parsers/connection_req_parser.h"
#include "selector.h"
#include "stateMachine.h"
#include "socks5nio.h"
#include "sctpnio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <sys/time.h>
#include <sys/signal.h>

#define TRUE 1
#define FALSE 0
#define PORT 1080
#define SCTP_PORT 8080
#define MAX_SOCKETS 20
#define MAX_PENDING_CONNECTIONS 5
#define SELECTOR_MAX_ELEMENTS 1024



#endif