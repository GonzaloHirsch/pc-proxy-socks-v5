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

#define DOH_PORT 80
#define BUFFERSIZE_DOH 1024
#define MAX_FDQN 256
#define DATA_MAX_SIZE 65536

#define REQ_MAX_SIZE 65536

char * request_generate(char * domain, int * length);
uint8_t * get_host_by_name(char * domain);