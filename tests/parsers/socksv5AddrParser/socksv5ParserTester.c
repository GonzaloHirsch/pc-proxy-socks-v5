#include "parsers/socks_5_addr_parser.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "io_utils/buffer.h"

int main (int argc, char * argv[]) {

    uint8_t sampleIPv4Address[] = {0x01, 0x04, 0x02, 0x01, 0x01, 0x00};
    uint8_t sampleDNAMEAddress[] = {0x03, 0x0E, 0x77, 0x77, 0x77, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x00};
    uint8_t sampleIPv6Address[] = {0x04, 0x04, 0x02, 0x01, 0x01, 0x14, 0x12, 0x11, 0x11, 0x24, 0x22, 0x21, 0x21, 0x34, 0x32, 0x31, 0x31, 0x00};
    
    buffer buff;
    uint8_t data[512] = {'h', 'o', 'l', 'a'};
    int errored;
    buffer_init(&buff, 511, data);

    for (char * b = sampleIPv4Address; *b; b++) {
        buffer_write(&buff, *b);
    }

    struct socks_5_addr_parser parser;
    socks_5_addr_parser_init(&parser);
    socks_5_addr_state st = socks_5_addr_consume_message(&buff, &parser, &errored);
    if (st == SOCKS5ADDR_DONE)
        printf("IPv4 Done\n");
    free_socks_5_addr_parser(&parser);

    buffer_init(&buff, 511, data);

    for (char * b = sampleIPv6Address; *b; b++) {
        buffer_write(&buff, *b);
    }

    socks_5_addr_parser_init(&parser);
    st = socks_5_addr_consume_message(&buff, &parser, &errored);
    if (st == SOCKS5ADDR_DONE)
        printf("IPv6 Done\n");
    free_socks_5_addr_parser(&parser);

    buffer_init(&buff, 511, data);

    for (char * b = sampleDNAMEAddress; *b; b++) {
        buffer_write(&buff, *b);
    }

    socks_5_addr_parser_init(&parser);
    st = socks_5_addr_consume_message(&buff, &parser, &errored);
    if (st == SOCKS5ADDR_DONE)
        printf("DNAME Done\n");
    free_socks_5_addr_parser(&parser);
}