#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "Utility.h"

//gcc -g ./tests/Utility.c ./tests/socksClient.c -o client

/**
 * Connect with netcat: (first the second line)
 * ncat -4 --proxy-type socks5 -v --proxy 127.0.0.1:1080 127.0.0.1 9090
 * ncat -l -k -v 9090 
 */