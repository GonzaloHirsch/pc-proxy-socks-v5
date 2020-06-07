#ifndef __DOH_RESPONSE_PARSER_H__
#define __DOH_RESPONSE_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "http_utils/httpUtils.h"
#include "io_utils/buffer.h"

// Warning! This assumes HTTP Versions of regex [0-9]\.[0-9]
// You've been warned --

typedef enum HTTPMessageState {
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
} HTTPMessageState;

typedef struct HTTPMessageParser * HTTPMessageParser;

HTTPMessageParser newHTTPMessageParser();

enum HTTPMessageState HTTPMessageReadNextByte(HTTPMessageParser p, const uint8_t b);
enum HTTPMessageState HTTPConsumeMessage(buffer * b, HTTPMessageParser p, int *errored);
const char * httpErrorString(const HTTPMessageParser p);
int HTTPDoneParsingMessage(HTTPMessageParser p, int * errored);

int getNumericHeaderValue(HTTPMessageParser p, const char * headerName);
HTTPHeader ** getHeaders(HTTPMessageParser p);
int getNumberOfHeaders(HTTPMessageParser p);
const char * getBody(HTTPMessageParser p);


// Free all HTTPMessageParser-Related memory

void freeHTTPMessageParser(HTTPMessageParser p);

#endif