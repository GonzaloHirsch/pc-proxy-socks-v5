#ifndef __CONNECTION_REQ_PARSER_H__
#define __CONNECTION_REQ_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum connection_req_state {
    CONN_REQ_VERSION,
    CONN_REQ_CMD,
    CONN_REQ_RSV,
    CONN_REQ_DSTADDR,
    CONN_REQ_BND_PORT,
    CONN_REQ_DONE,
    CONN_REQ_ERR_INV_VERSION,
    CONN_REQ_ERR_INV_CMD,
    CONN_REQ_ERR_INV_RSV,
    CONN_REQ_ERR_INV_DSTADDR,
    CONN_REQ_GENERIC_ERR
} connection_req_state;

typedef struct connection_req_parser * connection_req_parser;

connection_req_parser new_connection_req_parser();

enum connection_req_state connection_req_read_next_byte(connection_req_parser p, const uint8_t b);
enum connection_req_state connection_req_consume_message(buffer * b, connection_req_parser p, int *errored);
int connection_req_done_parsing(connection_req_parser p, int * errored);
// Free all connection_req_parser-Related memory

void free_connection_req_parser(connection_req_parser p);

connection_req_state get_con_state(connection_req_parser parser);

#endif