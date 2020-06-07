#ifndef __SOCKSV5_H__
#define __SOCKSV5_H__

#include "io_utils/buffer.h"
#include "parsers/HelloParser.h"
#include "parsers/SOCKS5AddrParser.h"
#include "parsers/ConnectionReqParser.h"
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

#define TRUE 1
#define FALSE 0
#define PORT 8888
#define MAX_SOCKETS 20
#define BUFFERSIZE 2048
#define MAX_PENDING_CONNECTIONS 5

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
    OP_READ = 0x00,
    OP_WRITE = 0x01
} ClientFdOperation;

/** General State Machine for a server connection */
typedef enum socks_v5state
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
     * 
     * Transitions:
     *  - CONNECTING -> While the server is establishing the connection with the destination of the request
     *  - REPLY -> When the connection is successful and our server can send the request
     *  - ERROR -> In case of an error
     * */
    CONNECTING,

    /**
     * State when sending/receiving the request to/from the destination server
     * 
     * Interests:
     *  - OP_READ -> Reading over the server response
     *  - OP_WRITE -> Writing to server the request
     * 
     * Transitions:
     *  - REPLY -> While the request is being sent and the response is still being copied
     *  - COPY -> When the server response is finished
     *  - ERROR -> In case of an error
     * */
    REPLY,

    /**
     * State when copying the response to the client fd
     * 
     * Interests:
     *  - OP_WRITE -> Writing to client fd
     * 
     * Transitions:
     *  - COPY -> While the server is still copying the response
     *  - DONE -> When the copy process is finished
     *  - ERROR -> In case of an error
     * */
    COPY,

    /** Terminal state */
    DONE,
    /** Terminal state */
    ERROR,
} socks_v5state;

typedef struct state_machine
{
    socks_v5state state;
} state_machine;

typedef state_machine *state_machine;

/** Used by the HELLO_READ and HELLO_WRITE states */
typdef struct hello_st
{
    /** Buffers used for IO */
    buffer *rb, *wb;
    /** Pointer to hello parser */
    HelloParser parser;
    /** Selected auth method */
    uint8_t method;
} hello_st;

/** Used by the RESOLVE state */
typdef struct resolve_st
{
    /** Buffers used for IO */
    buffer *rb, *wb;
    /** Pointer to hello parser */
    Socks5AddrParser parser;
    /** Resolved ip address */
    char * resolvedAddress;
} resolve_st;

/** Used by the REQUEST_READ state */
typdef struct request_st
{
    /** Buffer used for IO */
    buffer *rb;
    /** Pointer to hello parser */
    ConnectionReqParser parser;
} request_st;

typedef struct socks5
{
    /** maquinas de estados */
    state_machine stm;
    /** estados para el client_fd */
    union {
        hello_st hello;
        request_st request;
        copy copy;
    } client;
    /** estados para el origin_fd */
    union {
        connecting conn;
        copy copy;
    } orig;
} Socks5;

#endif