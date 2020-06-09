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

typedef struct hello_parser * hello_parser;

hello_parser new_hello_parser();

enum hello_state hello_read_next_byte(hello_parser p, const uint8_t b);
enum hello_state hello_consume_message(buffer * b, hello_parser p, int *errored);
int hello_done_parsing(hello_parser p, int * errored);

uint8_t get_n_auth(hello_parser p);
const uint8_t * get_auth_types(hello_parser p);

// Free all hello_parser-Related memory

void free_hello_parser(hello_parser p);


hello_state get_hello_state(hello_parser parser);

#endif