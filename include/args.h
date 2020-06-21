#ifndef ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdbool.h>
#include <sys/types.h>
#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <limits.h>    /* LONG_MIN et al */
#include <string.h>    /* memset */
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>     // AF_UNSPEC
#include <arpa/inet.h>      // inet_pton
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_USERS 10

struct users {
    char *name;
    char *pass;
};

struct doh {
    char           *host;
    char           *ip;
    unsigned short  port;
    char           *path;
    char           *query;
    uint16_t        buffer_size;
};

typedef struct socks5args_struct {
    char *              socks_addr;
    unsigned short      socks_port;
    int                 socks_family;
    struct addrinfo *   socks_addr_info;

    char *              mng_addr;
    unsigned short      mng_port;
    int                 mng_family;
    struct addrinfo *   mng_addr_info;

    bool            disectors_enabled;

    uint16_t socks5_buffer_size;
    uint16_t sctp_buffer_size;

    uint8_t timeout;

    struct doh      doh;
    struct users    users[MAX_USERS];
} socks5args_struct;

typedef struct socks5args_struct * socks5args;

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la seleccion humana. Puede cortar
 * la ejecución.
 */
void 
parse_args(const int argc, char * const*argv);

/**
 * Frees the memory used by the options
*/
void free_memory();

extern socks5args options;

#endif

