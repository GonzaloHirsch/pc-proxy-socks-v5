#ifndef __SCTP_ARGS_H__
#define __SCTP_ARGS_H__

#include <stdbool.h>
#include <stdio.h>  /* for printf */
#include <stdlib.h> /* for exit */
#include <limits.h> /* LONG_MIN et al */
#include <string.h> /* memset */
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>     // AF_UNSPEC
#include <arpa/inet.h>      // inet_pton
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef struct sctpClientArgs_struct {
    char *              mng_addr;
    unsigned short      mng_port;
    int                 mng_family;
    struct addrinfo *   mng_addr_info;
} sctpClientArgs_struct;

typedef struct sctpClientArgs_struct * sctpClientArgs;

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void 
parse_args(const int argc, char **argv);

void
free_memory();

extern sctpClientArgs args;

#endif

