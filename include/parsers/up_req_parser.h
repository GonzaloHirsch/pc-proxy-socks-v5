#ifndef __UPREQ_PARSER_H__
#define __UPREQ_PARSER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum up_req_state {
    UP_REQ_VERSION=0,
    UP_REQ_IDLEN,
    UP_REQ_ID,
    UP_REQ_PWLEN,
    UP_REQ_PW,
    UP_REQ_DONE,
    UP_ERROR_INV_VERSION,
    UP_ERROR_INV_IDLEN,
    UP_ERROR_INV_PWLEN,
    UP_ERROR_INV_AUTH
} up_req_state;

struct  up_req_parser {
    // public:      //
    uint8_t * uid;
    uint8_t * pw;
    uint8_t uidLen;
    uint8_t pwLen;
    // private:     //
    up_req_state state;
    uint8_t bytes_to_read;
};

typedef struct up_req_parser * up_req_parser;

// up_req_parser new_up_req_parser(); // deprecated

void up_req_parser_init(up_req_parser uprp);

enum up_req_state up_read_next_byte(up_req_parser p, const uint8_t b);
enum up_req_state up_consume_message(buffer * b, up_req_parser p, bool *errored);
int up_done_parsing(up_req_state st, bool * errored);
extern int
up_marshall(buffer *b, const uint8_t status);

// Free all up_req_parser-Related memory
void free_up_req_parser(up_req_parser p);

up_req_state get_up_req_state(up_req_parser parser);

#endif