#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>   
#include <arpa/inet.h>    
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 


#define TRUE 1
#define FALSE 0
#define PORT 1080
#define MAX_SOCKETS 20
#define BUFFERSIZE 2048
#define MAX_PENDING_CONNECTIONS 5


struct  buffer
{
    char * buffer;
    size_t len;
    size_t from;
};
