#ifndef __SCTPNIO_H__
#define __SCTPNIO_H__

/**
 * Interface for non blocking sctp IO
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include "selector.h"
#include "metrics.h"
#include "authentication.h"
#include "io_utils/buffer.h"
#include "io_utils/conversions.h"
#include "parsers/up_req_parser.h"
#include <arpa/inet.h>
// This header does not exist in OSX, so the compilation has to be done in Linux
#include <netinet/sctp.h>

#define SCTP_BUFFERSIZE 2048

/** Version of the negotiation */
#define SCTP_VERSION 0x01

/** 
 * States the SCTP process goes through 
 * Ideal flow is:
 *   REQUEST -> RESPONSE -> REQUEST -> RESPONSE ...
 * If error, the state is ERROR
*/
typedef enum SCTP_STATES
{
    SCTP_REQUEST,
    SCTP_RESPONSE,
    SCTP_ERROR
} SCTP_STATES;

/** Enum for the different types of areas to be used */
typedef enum TYPE
{
    TYPE_NOTYPE = 0x00,
    TYPE_USERS = 0x01,
    TYPE_METRICS = 0x02,
    TYPE_CONFIG = 0x03
} TYPE;

/** Enum for the different types of actions to be performed */
typedef enum CMD
{
    CMD_NOCMD = 0x00,
    CMD_LIST = 0x01,
    CMD_CREATE = 0x02,
    CMD_EDIT = 0x03
} CMD;

/** Struct to represent the message sent by the client */
typedef struct sctp_message
{
    CMD cmd;
    TYPE type;
} sctp_message;

/** Struct to represent authentication message sent by the client */
typedef struct sctp_auth_message
{
    uint8_t ulen;
    uint8_t plen;
    uint8_t *user;
    uint8_t *pass;
} sctp_auth_message;

/** Struct to hold information about a list user requet */
typedef struct sctp_user_list_message
{
    uint8_t user_count;
    uint8_t **users;
} sctp_user_list_message;

/** Struct to represent the state of a SCTP connection and the related information */
typedef struct sctp
{
    /** State of the sctp request */
    SCTP_STATES state;

    /** Client information */
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    int client_fd;

    /** Request information */
    sctp_message info;

    /** Union to represent the possible datagram sent by a user */
    union {
        sctp_auth_message user;
        sctp_user_list_message user_list;
    } datagram;

    /** Union to represent the parser the sctp connection is using in that moment */
    union {
        struct up_req_parser up_request;
    } paser;

    /** Buffers for reading and writing */
    buffer buffer_write, buffer_read;

    /** Flag to check if the user is logged in */
    int is_logged;

    /** Amount of references to this instance of the sctp struct, if the amount is 1, it can be deleted */
    unsigned references;

    /** Username of the active user performing changes */
    uint8_t *username;

    /** Next item in the pool */
    struct sctp *next;
} sctp;

/** Function to try to accept requests */
void sctp_passive_accept(struct selector_key *key);

/** Function to destroy the object pool */
void sctp_pool_destroy(void);

#endif
