#ifndef __DOH_RESPONSE_PARSER_H__
#define __DOH_RESPONSE_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "http_utils/http_utils.h"
#include "io_utils/buffer.h"

#define MAX_HEADERS 30
#define MAX_HEADER_NAME_LEN 40
#define MAX_HEADER_VALUE_LEN 100
#define BLOCK_SIZE 512

// Warning! This assumes HTTP Versions of regex [0-9]\.[0-9]
// You've been warned --

typedef enum http_message_state {
    HTTP_H, HTTP_T1, HTTP_T2, HTTP_P,
    HTTP_SLASH, HTTP_V1, HTTP_V2,
    HTTP_STATUS_CODE, HTTP_STATUS_MSG,
    HTTP_I1,
    HTTP_HK, HTTP_HV,
    HTTP_B,
    HTTP_I2,
    HTTP_F,
    HTTP_ERR_INV_MSG,
    HTTP_ERR_INV_STATUS_LINE,
    HTTP_ERR_INV_VERSION,
    HTTP_ERR_INV_STATUS_CODE,
    HTTP_ERR_INV_HK,
    HTTP_ERR_INV_HV,
    HTTP_ERR_INV_BODY
} http_message_state;

struct http_message_parser {
    // public:
    // list of pairs or hash?
    http_message_state state;
    uint8_t version[5];
    uint8_t status[4];
    uint8_t * body;
    int headers_num;
    http_header * headers[MAX_HEADERS];
    // private:
    uint8_t header_name[MAX_HEADER_NAME_LEN+1];
    uint8_t header_value[MAX_HEADER_VALUE_LEN+1];
    uint8_t * cursor;
    int bytes_to_read;
    int body_len;
    uint8_t buff[BLOCK_SIZE];
};


typedef struct http_message_parser * http_message_parser;

// http_message_parser new_http_message_parser(); // deprecated

void http_message_parser_init(http_message_parser hmp); 

enum http_message_state http_message_read_next_byte(http_message_parser p, const uint8_t b);
enum http_message_state http_consume_message(buffer * b, http_message_parser p, int *errored);
const char * http_error_string(const http_message_parser p);
int http_done_parsing_message(http_message_parser p, int * errored);

int get_numeric_header_value(http_message_parser p, const char * header_name);
http_header ** get_headers(http_message_parser p);
int get_number_of_headers(http_message_parser p);
const char * get_body(http_message_parser p);


// Free all http_message_parser-Related memory

void free_http_message_parser(http_message_parser p);

http_message_state get_http_state(http_message_parser parser);

#endif