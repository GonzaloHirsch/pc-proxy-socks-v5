#ifndef __SOCKSV5_H__
#define __SOCKSV5_H__

#include "../include/socksv5.h"
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
#include <sys/time.h> 

typedef enum AuthOType
{
    NO_AUTH = 0x00,
    USER_PASSWORD = 0x02
} AuthType;

typedef enum AddrType
{
    IPv4 = 0x01,
    DOMAIN_NAME = 0x03,
    IPv6 = 0x04
} AddrType;

typedef enum ClientFdOperation
{
    OP_READ = 0x01,
    OP_WRITE = 0x02
} ClientFdOperation;

/* General State Machine for a server connection */
enum socks_v5state
{
    /**
     * State when receiving the client initial "hello" and processing
     * 
     * Interests:
     *  - OP_READ -> Reading over the client fd
     * 
     * Transitions:
     *  - HELLO_READ -> While the message is not complete
     *  - HELLO_WRITE -> When the message is complete
     *  - ERROR -> In case of an error
     * */
    HELLO_READ,

    /**
     * State when sending the client initial "hello" response
     * 
     * Interests:
     *  - OP_WRITE -> Writing over the client fd
     * 
     * Transitions:
     *  - HELLO_WRITE -> While there are bytes to send
     *  - REQUEST_READ -> When all the bytes have been transfered
     *  - ERROR -> In case of an error
     * */
    HELLO_WRITE,

    /**
     * State when receiving the client request
     * 
     * Interests:
     *  - OP_READ -> Reading over the client fd
     * 
     * Transitions:
     *  - REQUEST_READ -> While the message is not complete
     *  - RESOLVE -> When the message is complete
     *  - ERROR -> In case of an error
     * */
    REQUEST_READ,

    /**
     * State when the client uses a FQDN to connect to another server, the name must be resolved
     * 
     * Interests:
     *  - OP_READ -> Reading over the resolution response fd
     *  - OP_WRITE -> Writing to the DNS server the request
     * 
     * Transitions:
     *  - RESOLVE -> While the name is not resolved
     *  - CONNECTING -> When the name is resolved and the IP address obtained
     *  - ERROR -> In case of an error
     * */
    RESOLVE,

    /**
     * State when connecting to the server the client request is headed to
     * 
     * Interests:
     *  - OP_WRITE -> Writing the request to the server
     * 
     * Transitions:
     *  - CONNECTING -> While the 
     *  - REPLY -> When the name is resolved and the IP address obtained
     *  - ERROR -> In case of an error
     * */
    CONNECTING,
    REPLY,
    COPY,

    // estados terminales
    DONE,
    ERROR,
};


#define TRUE 1
#define FALSE 0
#define PORT 8888
#define MAX_SOCKETS 20
#define BUFFERSIZE 2048
#define MAX_PENDING_CONNECTIONS 5


struct  buffer
{
    char * buffer;
    size_t len;
    size_t from;
};

#endif