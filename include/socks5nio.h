#ifndef __SOCKS5NIO_H__
#define __SOCKS5NIO_H__

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

#define N(x) (sizeof(x) / sizeof((x)[0]))
/** TODO: define appropiate size */
#define BUFFERSIZE 4096

/** Function to try to accept requests */
void socksv5_passive_accept(struct selector_key *key);

/** Function to free the pool of socks5 instances */
void socksv5_pool_destroy(void);

/** General state machine */
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
};

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
    struct hello_parser parser;
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

/** Struct for the request information */
typedef struct socks5_request_info
{
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t dstPort[2];
    uint8_t type;
    uint8_t *addr;
    uint8_t addrLen;
} socks5_request_info;

/** Struct for origin server information */
typedef struct socks5_origin_info
{
    /** Origin server address info */
    struct sockaddr_storage origin_addr;
    socklen_t origin_addr_len;
} socks5_origin_info;


// Struct used to store all the relevant info for a socket.
typedef struct socks5
{
    /** State machine for the connection */
    struct state_machine stm;

    /** File descriptor number for the client */
    int client_fd;
    /** File descriptor number for the origin */
    int origin_fd;

    /** States for the client fd */
    union {
        hello_st hello;
        request_st request;
        copy_st copy;
    } client;
    /** States for the origin fd */
    union {
        connecting_st conn;
        reply_st reply;
    } orig;

    /** Address info for the origin server */
    struct addrinfo *origin_resolution;

    /** Amount of references to this instance of the socks5 struct, if the amount is 1, it can be deleted */
    unsigned references;

    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;

    /** Buffers for reading and writing */
    buffer read_buffer, write_buffer;

    /** Next item in the pool */
    struct socks5 *next;

    /** Authentication method */
    uint8_t auth;    

    /** Information about the request sent */
    struct socks5_request_info request_info;

    /** Information about the origin server */
    struct socks5_origin_info origin_info;
} socks5;


#endif