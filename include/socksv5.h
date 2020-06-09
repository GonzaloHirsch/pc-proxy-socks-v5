#ifndef __SOCKSV5_H__
#define __SOCKSV5_H__

#include "io_utils/buffer.h"
#include "parsers/hello_parser.h"
#include "parsers/socks_5_addr_parser.h"
#include "parsers/connection_req_parser.h"
#include "selector.h"
#include "stateMachine.h"
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
#include <sys/signal.h>

#define TRUE 1
#define FALSE 0
#define PORT 8888
#define MAX_SOCKETS 20
#define BUFFERSIZE 2048
#define MAX_PENDING_CONNECTIONS 5
#define SELECTOR_MAX_ELEMENTS 1024

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
    CLI_OP_READ = 0x00,
    CLI_OP_WRITE = 0x01
} ClientFdOperation;

/** Used by the HELLO_READ and HELLO_WRITE states */
typedef struct hello_st
{
    /** Buffers used for IO */
    buffer *rb, *wb;
    /** Pointer to hello parser */
    hello_parser parser;
    /** Selected auth method */
    uint8_t method;
} hello_st;

/** Used by the RESOLVE state */
typedef struct resolve_st
{
    /** Buffers used for IO */
    buffer *rb, *wb;
    /** Pointer to the resolve parser */
    socks_5_addr_parser parser;
    /** Resolved ip address */
    char *resolvedAddress;
} resolve_st;

/** Used by the REQUEST_READ state */
typedef struct request_st
{
    /** Buffer used for IO */
    buffer *rb;
    /** Pointer to request parser */
    connection_req_parser parser;
} request_st;

/** Used by the COPY state */
typedef struct copy_st
{
    /** Buffer used for IO */
    buffer *rb;
    
    
} copy_st;

/** Used by the CONNECTING state */
typedef struct connecting_st
{
    /** Buffer used for IO */
    buffer *rb;
    
    
} connecting_st;

/** Used by REPLY the state */
typedef struct reply_st
{
    /** Buffer used for IO */
    buffer *rb;
    
    
} reply_st;


// Struct used to store all the relevant info for a socket.
typedef struct Socks5
{
    //maquinas de estados
    struct state_machine stm;
    
    //estados para el client_fd
    union {
        hello_st hello;
        request_st request;
        copy_st copy;
    } client;
    // estados para el origin_fd 
    union {
        connecting_st conn;
        reply_st reply;
    } orig;

    buffer * writeBuffer;
    buffer * readBuffer;

} Socks5;

#endif