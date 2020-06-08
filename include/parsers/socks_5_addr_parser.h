#ifndef __SOCKS_5_ADDRESS_PARSER_H__
#define __SOCKS_5_ADDRESS_PARSER_H__

#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#include "io_utils/buffer.h"

typedef enum socks_5_addr_state {
    SOCKS5ADDR_TYPE,
    SOCKS5ADDR_IP_V4,
    SOCKS5ADDR_DNAME_LEN,
    SOCKS5ADDR_DNAME,
    SOCKS5ADDR_IP_V6,
    SOCKS5ADDR_DONE,
    SOCKS5ADDR_ERR_INV_TYPE,
    SOCKS5ADDR_ERR_INV_ADDRESS
} socks_5_addr_state;

typedef struct socks_5_addr_parser * socks_5_addr_parser;

socks_5_addr_parser new_socks_5_addr_parser();

enum socks_5_addr_state socks_5_addr_read_next_byte(socks_5_addr_parser p, const uint8_t b);
enum socks_5_addr_state socks_5_addr_consume_message(buffer * b, socks_5_addr_parser p, int *errored);
int socks_5_addr_done_parsing(socks_5_addr_parser p, int * errored);

const char * get_socks_5_address(socks_5_addr_parser p);
const int get_socks_5_type(socks_5_addr_parser p);

// Free all socks_5_addr_parser-Related memory
void free_socks_5_addr_parser(socks_5_addr_parser p);

#endif