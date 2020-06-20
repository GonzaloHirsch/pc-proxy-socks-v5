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

#define N(x) (sizeof(x) / sizeof((x)[0]))

void log_request(const int status, const uint8_t *username, const struct sockaddr *clientaddr, const struct sockaddr *originaddr, const uint8_t *fqdn);
void log_password(const uint8_t *owner, const int protocol, const struct sockaddr *originaddr, const uint8_t *fqdn, const uint8_t *u, const uint8_t *p);

#endif