#ifndef __CONNECTION_REQ_PARSER_H__
#define __CONNECTION_REQ_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"
#include "parsers/socks_5_addr_parser.h"

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

typedef struct connection_req {
    // public:
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t dstPort[2];
} connection_req;


struct connection_req_parser {
    // public:
    connection_req finalMessage;
    connection_req_state state;
    // private:
    socks_5_addr_parser socks_5_addr_parser;
    unsigned int bytes_to_read;
};

typedef struct connection_req_parser * connection_req_parser;

// connection_req_parser new_connection_req_parser(); // deprecated

void connection_req_parser_init(connection_req_parser crp);

enum connection_req_state connection_req_read_next_byte(connection_req_parser p, const uint8_t b);
enum connection_req_state connection_req_consume_message(buffer * b, connection_req_parser p, bool *errored);
int connection_req_done_parsing(connection_req_parser p, bool * errored);
// Free all connection_req_parser-Related memory

void free_connection_req_parser(connection_req_parser p);

connection_req_state get_con_state(connection_req_parser parser);

#endif