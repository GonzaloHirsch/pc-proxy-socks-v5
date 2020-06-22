#ifndef __HTTP_AUTH_PARSER__
#define __HTTP_AUTH_PARSER__

#define MAX_HTTP_LINE 1024

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"


typedef enum http_auth_state{
    HTTPA_NL,
    HTTPA_READ,
    HTTPA_IN,
    HTTPA_A,
    HTTPA_U,
    HTTPA_T,
    HTTPA_H,
    HTTPA_O,
    HTTPA_R,
    HTTPA_I,
    HTTPA_Z,
    HTTPA_A2,
    HTTPA_T2,
    HTTPA_I2,
    HTTPA_O2,
    HTTPA_N,
    HTTPA_S1,
    HTTPA_S2,
    HTTPA_R_S,
    HTTPA_G_ENCODING,
    HTTPA_G_CONTENT,
    HTTPA_BNL,
    HTTPA_2BNL,
    HTTPA_F,
    HTTPA_ERROR
} http_auth_state;


typedef struct http_auth_parser * http_auth_parser;


struct http_auth_parser{

    uint8_t buffer[MAX_HTTP_LINE];
    uint8_t * cursor;
    uint8_t * encoding;
    uint8_t * content;    
    http_auth_state state;
    int in_auth;

};


void http_auth_init(http_auth_parser p);

http_auth_state http_auth_consume_msg(buffer * b, http_auth_parser p, int * errored);

int http_auth_done_parsing(http_auth_parser p);


void free_http_auth_parser(http_auth_parser p);



#endif
