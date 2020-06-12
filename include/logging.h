#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>


#define N(x) (sizeof(x)/sizeof((x)[0]))

// To get the time in a human readable form https://www.tutorialspoint.com/c_standard_library/c_function_strftime.htm
// To write to a file https://www.programiz.com/c-programming/c-file-input-output

// #ifndef for tests to avoid duplicate definition of constants
const char * REQUESTS_FILE = "./logs/requests.log";
const char * CREDENTIALS_FILE = "./logs/credentials.log";

void init_requests_log();
void init_credentials_log();

void log_request(uint8_t * client_address, uint8_t * client_port, uint8_t * origin_address, uint8_t * origin_port);
void log_credential(uint8_t * user, uint8_t * pass);

void close_requests_log();
void close_credentials_log();

#endif
