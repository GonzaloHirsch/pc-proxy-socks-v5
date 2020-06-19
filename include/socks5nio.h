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
#include "parsers/up_req_parser.h"
#include "parsers/http_message_parser.h"
#include "selector.h"
#include "stateMachine.h"
#include "authentication.h"
#include "args.h"


#define N(x) (sizeof(x) / sizeof((x)[0]))
/** TODO: define appropiate size */
#define BUFFERSIZE 4096
#define MAX_IPS 10
#define USER_PASS_FILE "./serverdata/user_pass.txt"

// OSx users don't have the MSG_NOSIGNAL defined
// We define it as 0 so that systems that don't offer this signal don't use it
// Source: https://github.com/lpeterse/haskell-socket/issues/8
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0x0
#endif

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
     *  - USERPASS_READ -> When all the bytes have been transfered
     *  - ERROR -> In case of an error
     * */
    HELLO_WRITE,

    /**
     * State when receiving the user and password
     * 
     * Interests:
     *  -OP_READ -> Read the info sent by the user
     * 
     * Transitions:
     *  - USSERPASS_READ -> While there are bytes being read
     *  - USSERPASS_WRITE -> When all the bytes have been read and processed
     *  - ERROR -> In case of an error
     * */
    USERPASS_READ,

    /**
     * State when receiving the user and password
     * 
     * Interests:
     *  -OP_WRITE -> Write if u+p is valid or not
     * 
     * Transitions:
     *  - USSERPASS_READ -> While there are bytes being sent
     *  - REQUEST_READ -> If u+p is valid
     *  - ERROR -> In case of u+p invalid or other error
     * */
    USERPASS_WRITE,

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

typedef enum AuthStatus
{
    AUTH_SUCCESS = 0x00,
    AUTH_FAILURE = 0xFF,
} AuthStatus;

typedef enum AddrType
{
    IPv4 = 0x01,
    DOMAIN_NAME = 0x03,
    IPv6 = 0x04,
} AddrType;

typedef enum ClientFdOperation
{
    CLI_OP_READ = 0x00,
    CLI_OP_WRITE = 0x01
} ClientFdOperation;

typedef enum AddressSize {
    IP_V4_ADDR_SIZE = 4,
    IP_V6_ADDR_SIZE = 16,
    PORT_SIZE = 2,
} AddressSize;

typedef enum ConnectionResponse {
    CONN_RESP_REQ_GRANTED = 0x00,
    CONN_RESP_GENERAL_FAILURE,
    CONN_RESP_NOT_ALLOWED_BY_RULESET,
    CONN_RESP_NET_UNREACHABLE,
    CONN_RESP_HOST_UNREACHABLE,
    CONN_RESP_REFUSED_BY_DEST_HOST,
    CONN_RESP_TTL_EXPIRED,
    CONN_RESP_CMD_NOT_SUPPORTED,
    CONN_RESP_ADDR_TYPE_NOT_SUPPORTED
}ConnectionResponse;

typedef enum ConnectionState {
    CONN_INPROGRESS, 
    CONN_FAILURE,
    CONN_SUCCESS
}ConnectionState;

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

/** Used by the USERPASS_READ and USERPASS_WRITE states */
typedef struct userpass_st
{
    /** Buffers used for IO */
    buffer *rb, *wb;
    /** Pointer to hello parser */
    struct up_req_parser parser;
    /** Selected user */
    uint8_t * user;
    /** Selected password */
    uint8_t * password;
} userpass_st;

/** Used by the REQUEST_READ state */
typedef struct request_st
{
    /** Buffer used for IO */
    buffer *rb;
    /** Pointer to request parser */
    struct connection_req_parser parser;
} request_st;

/** Used by the RESOLVE state */
typedef struct resolve_st
{
    /** Buffer used for IO */
    buffer *rb, *wb;
    /** File descriptor of the doh server */
    int doh_fd;
    struct http_message_parser parser;
    struct sockaddr_in serv_addr;
    unsigned int conn_state;
} resolve_st;

/** Used by the COPY state */
typedef struct copy_st
{   
    /** File descriptor */
    int fd;
    /** Reading buffer */
    buffer * rb;
    /** Writing buffer */
    buffer * wb;
    /** Interest of the copy */
    fd_interest interest;
    /** Pointer to the structure of the opposing copy state*/
    struct copy_st * other_copy;
} copy_st;

/** Used by the CONNECTING state */
typedef struct connecting_st
{
    /** Buffer used for IO */
    buffer *rb;
    unsigned int first_working_ip_index;

} connecting_st;

/** Struct for origin server information */
typedef struct socks5_origin_info
{
    /** Origin server address info */
    struct sockaddr_storage origin_addr;
    socklen_t origin_addr_len;

    /** Prefered ip type */
    uint8_t ip_selec;
    /** IPv4 Addresses info */
    uint8_t ipv4_addrs[MAX_IPS][IP_V4_ADDR_SIZE];
    uint8_t ipv4_c;
    /** Ipv6 Addresses info */
    uint8_t ipv6_addrs[MAX_IPS][IP_V6_ADDR_SIZE];
    uint8_t ipv6_c;

    /** Port info */
    uint8_t port[PORT_SIZE];
    
    /** Origin info in case we need to relve */
    uint8_t * resolve_addr;
    uint8_t resolve_addr_len;


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
        userpass_st userpass;
        request_st request;
        copy_st copy;
    } client;
    /** States for the origin fd */
    union {
        resolve_st resolve;
        connecting_st conn;
        copy_st copy;
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

    /** Credentials */
    uint8_t * username;    

    /** Information about the origin server */
    struct socks5_origin_info origin_info;
} socks5;

#include "dohClient.h"
#endif