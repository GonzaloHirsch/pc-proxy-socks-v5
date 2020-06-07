#ifndef __HELLO_PARSER_H__
#define __HELLO_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum HelloState {
    HELLO_VER,
    HELLO_NAUTH,
    HELLO_AUTH,
    HELLO_DONE,
    HELLO_ERR_INV_VERSION,
    HELLO_ERR_UNASSIGNED_METHOD,
    HELLO_ERR_UNSUPPORTED_METHOD,
} HelloState;

typedef struct HelloParser * HelloParser;

HelloParser newHelloParser();

enum HelloState helloReadNextByte(HelloParser p, const uint8_t b);
enum HelloState helloConsumeMessage(buffer * b, HelloParser p, int *errored);
int helloDoneParsing(HelloParser p, int * errored);
// Free all HelloParser-Related memory

void freeHelloParser(HelloParser p);

#endif