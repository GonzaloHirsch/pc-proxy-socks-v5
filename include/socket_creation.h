#ifndef __SOCKET_CREATION_H__
#define __SOCKET_CREATION_H__

#include "socks5nio/socks5nio.h"
#include "sctpnio/sctpnio.h"
#include "args.h"
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
#include "socksv5.h"

int create_management_socket_6(struct sockaddr_in6 * addr);
int create_management_socket_4(struct sockaddr_in * addr);
int create_master_socket_6(struct sockaddr_in6 * addr);
int create_master_socket_4(struct sockaddr_in * addr);
void create_management_socket_all(int *management_socket_4, int *management_socket_6);
void create_master_socket_all(int *master_socket_4, int *master_socket_6);

#endif