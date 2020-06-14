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
#include "selector.h"
#include "buffer.h"
#include <arpa/inet.h>

#define BUFFERSIZE 2048

/** Version of the negotiation */
#define VERSION 0x01;

/** Enum for the different types of areas to be used */
typedef enum TYPE {
    TYPE_USERS = 0x00,
    TYPE_METRICS = 0x01,
    TYPE_CONFIG = 0x02
} TYPE;

/** Enum for the different types of actions to be performed */
typedef enum CMD {
    CMD_LIST = 0x00,
    CMD_CREATE = 0x01,
    CMD_EDIT = 0x02
} CMD;

typedef struct sctp
{
    /** Client information */
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    int client_fd;

    /** Buffers for reading and writing */
    buffer buffer_write, buffer_read;

    /** Flag to check if the user is logged in */
    int is_logged;

    /** Amount of references to this instance of the sctp struct, if the amount is 1, it can be deleted */
    unsigned references;

    /** Username of the active user performing changes */
    char * username;

    /** Next item in the pool */
    struct sctp *next;
} sctp;

/** Function to try to accept requests */
void sctp_passive_accept(struct selector_key *key);

/** Function to destroy the object pool */
void sctp_pool_destroy(void);

#endif
