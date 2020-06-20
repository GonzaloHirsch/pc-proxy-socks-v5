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

void log_request(const int status, const struct sockaddr *clientaddr, const struct sockaddr *originaddr);

#endif