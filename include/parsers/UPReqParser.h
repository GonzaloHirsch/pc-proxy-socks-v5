#ifndef __UPREQ_PARSER_H__
#define __UPREQ_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum UPReqState {
    UP_REQ_VERSION,
    UP_REQ_IDLEN,
    UP_REQ_ID,
    UP_REQ_PWLEN,
    UP_REQ_PW,
    UP_REQ_DONE,
    UP_ERROR_INV_VERSION,
    UP_ERROR_INV_IDLEN,
    UP_ERROR_INV_PWLEN,
    UP_ERROR_INV_AUTH
} UPReqState;

typedef struct UPReqParser * UPReqParser;

UPReqParser newUPReqParser();

enum UPReqState upReadNextByte(UPReqParser p, const uint8_t b);
enum UPReqState consumeMessage(buffer * b, UPReqParser p, int *errored);
int upDoneParsing(UPReqParser p, int * errored);
// Free all UPReqParser-Related memory

void freeUPReqParser(UPReqParser p);

#endif