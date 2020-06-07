#ifndef __CONNECTION_REQ_PARSER_H__
#define __CONNECTION_REQ_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum ConnectionReqState {
    CONN_REQ_VERSION,
    CONN_REQ_CMD,
    CONN_REQ_RSV,
    CONN_REQ_DSTADDR_TYPE,
    CONN_REQ_DSTADDR_ADDR,
    CONN_REQ_DSTADDR_PORT
} ConnectionReqState;

typedef struct ConnectionReqParser * ConnectionReqParser;

ConnectionReqParser newConnectionReqParser();

enum ConnectionReqState connectionReqReadNextByte(ConnectionReqParser p, const uint8_t b);
enum ConnectionReqState connectionReqConsumeMessage(buffer * b, ConnectionReqParser p, int *errored);
int connectionReqDoneParsing(ConnectionReqParser p, int * errored);
// Free all ConnectionReqParser-Related memory

void freeConnectionReqParser(ConnectionReqParser p);

#endif