#ifndef __SCTPCLIENT_H__
#define __SCTPCLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include "Utility.h"

#define DIVIDER printf("\n\n-------------------------------------------------------\n\n");
#define SUBDIVIDER printf("\n\n---------------------------\n\n");

#define N(x) (sizeof(x)/sizeof((x)[0]))

#define MAX_BUFFER 1024
#define MAX_OPT_SIZE 4
#define MAX_CONF_SIZE 1
#define MAX_CONF_VALUE_SIZE 5

typedef enum Options {
    OPT_EXIT = 0x01,
    OPT_LIST_USERS = 0x02,
    OPT_CREATE_USER = 0x03,
    OPT_SHOW_METRICS = 0x04,
    OPT_SHOW_CONFIGS = 0x05,
    OPT_EDIT_CONFIG = 0x06
} Options;

typedef enum Errors {
    ERR_NO_ERR = 0x00,
    ERR_GENERAL = 0x01,
    ERR_INVALID_TYPE = 0x02,
    ERR_INVALID_CMD = 0x03,
    ERR_INVALID_DATA = 0x04,
    ERR_INVALID_NUMERIC_DATA = 0x05
} Errors;

typedef enum Configs{
    CONF_SOCKS5_BUFF = 0x01,
    CONF_SCTP_BUFF = 0x02,
    CONF_DISECTORS = 0x03,
    CONF_EXIT = 0x64
} Configs;

void die_with_message(char *msg);
void determine_error(int err);
void handle_invalid_value();
void handle_undefined_command();

extern int serverSocket;

#endif