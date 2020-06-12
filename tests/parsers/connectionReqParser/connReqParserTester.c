#include "parsers/connection_req_parser.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "io_utils/buffer.h"

#define N(x) (sizeof(x)/sizeof((x)[0]))

int main (int argc, char * argv[]) {

    uint8_t sampleIPv4Address[] = {0x05, 0x01, 0x00, 0x01, 0x04, 0x02, 0x01, 0x01, 0x77, 0x77, 0x00};
    uint8_t sampleDNAMEAddress[] = {0x05, 0x01, 0x00, 0x03, 0x0E, 0x77, 0x77, 0x77, 0x2e, 0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x77, 0x77, 0x00};
    uint8_t sampleIPv6Address[] = {0x05, 0x01, 0x00, 0x04, 0x04, 0x02, 0x01, 0x01, 0x14, 0x12, 0x11, 0x11, 0x24, 0x22, 0x21, 0x21, 0x34, 0x32, 0x31, 0x31, 0x77, 0x77, 0x00};
    
    buffer buff;
    uint8_t data[512] = {'h', 'o', 'l', 'a'};
    int errored;
    buffer_init(&buff, 511, data);

    for (uint8_t * b = sampleIPv4Address; b - sampleIPv4Address < N(sampleIPv4Address) ; b++) {
        buffer_write(&buff, *b);
    }
    struct connection_req_parser pr;
    connection_req_parser_init(&pr);

    connection_req_state st = connection_req_consume_message(&buff, &pr, &errored);
    if (st = CONN_REQ_DONE)
        printf("IPv4 Done!\n");

    buffer_init(&buff, 511, data);

    for (uint8_t * b = sampleIPv6Address; b - sampleIPv6Address < N(sampleIPv6Address) ; b++) {
        buffer_write(&buff, *b);
    }
    connection_req_parser_init(&pr);
    st = connection_req_consume_message(&buff, &pr, &errored);
    if (st = CONN_REQ_DONE)
        printf("IPv6 Done!\n");

    buffer_init(&buff, 511, data);

    for (uint8_t * b = sampleDNAMEAddress; b - sampleDNAMEAddress < N(sampleDNAMEAddress) ; b++) {
        buffer_write(&buff, *b);
    }
    connection_req_parser_init(&pr);
    st = connection_req_consume_message(&buff, &pr, &errored);
    if (st = CONN_REQ_DONE)
        printf("DNAME Done!\n");
    free_connection_req_parser(&pr);
}