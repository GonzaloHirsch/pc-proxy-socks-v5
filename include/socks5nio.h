#ifndef __NETUTILS_H__
#define __NETUTILS_H__

//------------------------SACADO DE EJEMPLO PARCIAL---------------------------
// https://campus.itba.edu.ar/webapps/blackboard/execute/content/file?cmd=view&content_id=_238320_1&course_id=_12752_1&framesetWrapped=true

#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memset
#include <assert.h> // assert
#include <errno.h>
#include <time.h>
#include <unistd.h> // close
#include <pthread.h>
#include <arpa/inet.h>

#include "io_utils/buffer.h"
#include "parsers/hello_parser.h"
#include "parsers/socks_5_addr_parser.h"
#include "parsers/connection_req_parser.h"
#include "selector.h"
#include "stateMachine.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))
#define MAX_STATES 9

/** Function to try to accept requests */
void socksv5_passive_accept(struct selector_key *key);

/** Function to free the pool of socks5 instances */
void socksv5_pool_destroy(void);

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
typedef struct socks5
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

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;

    buffer * writeBuffer;
    buffer * readBuffer;

} socks5;

#endif