#ifndef __SOCKS_5_ADDRESS_PARSER_H__
#define __SOCKS_5_ADDRESS_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum Socks5AddrState {
    SOCKS5ADDR_TYPE,
    SOCKS5ADDR_IP_V4,
    SOCKS5ADDR_DNAME_LEN,
    SOCKS5ADDR_DNAME,
    SOCKS5ADDR_IP_V6,
    SOCKS5ADDR_DONE,
    SOCKS5ADDR_ERR_INV_TYPE,
    SOCKS5ADDR_ERR_INV_ADDRESS
} Socks5AddrState;

typedef struct Socks5AddrParser * Socks5AddrParser;

Socks5AddrParser newSocks5AddrParser();

enum Socks5AddrState socks5AddrReadNextByte(Socks5AddrParser p, const uint8_t b);
enum Socks5AddrState socks5AddrConsumeMessage(buffer * b, Socks5AddrParser p, int *errored);
int socks5AddrDoneParsing(Socks5AddrParser p, int * errored);

const char * getSocks5Address(Socks5AddrParser p);
const int getSocks5Type(Socks5AddrParser p);

// Free all Socks5AddrParser-Related memory
void freeSocks5AddrParser(Socks5AddrParser p);

#endif