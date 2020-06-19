#ifndef __SCTP_ARGS_H__
#define __SCTP_ARGS_H__

#include <stdbool.h>
#include <stdio.h>  /* for printf */
#include <stdlib.h> /* for exit */
#include <limits.h> /* LONG_MIN et al */
#include <string.h> /* memset */
#include <errno.h>
#include <getopt.h>

struct sctpClientArgs {
    char *          mng_addr;
    unsigned short  mng_port;
};

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void 
parse_args(const int argc, char **argv, struct sctpClientArgs *args);

#endif

