#ifndef __DOHCLIENT__H
#define __DOHCLIENT__H


#include <netdb.h>
#include <stdio.h>
#include <stdlib.h> // malloc
#include <string.h> // memset
#include <assert.h> // assert
#include <errno.h>
#include <time.h>
#include <unistd.h> // close
#include <pthread.h>
#include <arpa/inet.h>
#include "parsers/http_message_parser.h"
#include "dnsPacket.h"

#define DOH_PORT 80
#define BUFFERSIZE_DOH 1024
#define MAX_FDQN 256
#define DATA_MAX_SIZE 65536

#define REQ_MAX_SIZE 65536

char * request_generate(char * domain, int * length, int qtype);
uint8_t * get_host_by_name(char * domain);

void parse_to_crlf(uint8_t * response, size_t *size);
void receive_dns_parse(char * final_buffer, char * domain, int buf_size, struct socks5 * s, int * errored);


#endif