#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include <netinet/in.h>
#include "netutils.h"
#include "./socks5nio/socks5nio.h"

#define N(x) (sizeof(x) / sizeof((x)[0]))

void log_request(const int status, const uint8_t *username, const struct sockaddr *clientaddr, const struct sockaddr *originaddr, const uint8_t *fqdn);
void log_disector(const uint8_t *owner, struct socks5_origin_info orig_info, const uint8_t *u, const uint8_t *p);
#endif