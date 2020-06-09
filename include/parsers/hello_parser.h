#ifndef __HELLO_PARSER_H__
#define __HELLO_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum hello_state {
    HELLO_VER,
    HELLO_NAUTH,
    HELLO_AUTH,
    HELLO_DONE,
    HELLO_ERR_INV_VERSION,
    HELLO_ERR_INV_NAUTH,
    HELLO_ERR_UNASSIGNED_METHOD,
    HELLO_ERR_UNSUPPORTED_METHOD,
} hello_state;

struct hello_parser {
    // public:
    uint8_t nauth;
    uint8_t * auth;
    // private:
    unsigned int bytes_to_read;
    hello_state state;
};


/** Constant for no authentication required for the user */
static const uint8_t SOCKS_HELLO_NOAUTHENTICATION_REQUIRED = 0x00;
/*
 * If the selected METHOD is X'FF', none of the methods listed by the
   client are acceptable, and the client MUST close the connection.
 */
static const uint8_t SOCKS_HELLO_NO_ACCEPTABLE_METHODS = 0xFF;

typedef struct hello_parser * hello_parser;

// hello_parser new_hello_parser(); // deprecated

void hello_parser_init(hello_parser hp);

enum hello_state hello_read_next_byte(hello_parser p, const uint8_t b);
enum hello_state hello_consume(buffer * b, hello_parser p, bool *errored);
int hello_done_parsing(hello_parser p, bool * errored);
int hello_is_done(hello_state st, bool * errored);

uint8_t get_n_auth(hello_parser p);
const uint8_t * get_auth_types(hello_parser p);

int hello_marshall(buffer *b, const uint8_t method);

// Free all hello_parser-Related memory

void free_hello_parser(hello_parser p);


hello_state get_hello_state(hello_parser parser);

#endif