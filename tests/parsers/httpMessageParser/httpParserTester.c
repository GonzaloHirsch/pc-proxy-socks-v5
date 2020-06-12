#include "http_utils/http_utils.h"
#include "parsers/http_message_parser.h"
#include "io_utils/buffer.h"
#include <ctype.h>
#include <unistd.h>

int main (int argc, char * argv[]) {
    char  sampleResponse []= 
    "HTTP/1.1 200 OK\n"
    "Server: nginx/1.10.0\n"
    "Date: Thu, 04 Jun 2020 21:55:08 GMT\n"
    "Content-Type: application/dns-message\n"
    "Content-Length: 44\n"
    "Connection: close\n"
    "cache-control: max-age=300\n"
    "X-Resolved-By: 146.112.41.2:443\n"
    "\n"
    "afsdasdffsadasdffadsadfsfadsafsdasfdasfdafsd\n\n";

    buffer buff;
    uint8_t data[512] = {'h', 'o', 'l', 'a'};
    int errored;
    buffer_init(&buff, 511, data);

    for (char * b = sampleResponse; *b; b++) {
        buffer_write(&buff, *b);
    }

    struct http_message_parser parser;
    http_message_parser_init(&parser);
    http_message_state st = http_consume_message(&buff, &parser, &errored);
    if (HTTP_F)
        printf("Done\n");
    free_http_message_parser(&parser);
}