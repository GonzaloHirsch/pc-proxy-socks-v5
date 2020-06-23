#ifndef __NETUTILS_H__
#define __NETUTILS_H__

//------------------------SACADO DE PARCHE 5---------------------------

#include <netinet/in.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <arpa/inet.h>

#include "io_utils/buffer.h"

#define SOCKADDR_TO_HUMAN_MIN (INET6_ADDRSTRLEN + 5 + 1)

// This definition is because it cannot compile in OSX otherwise
// Sources: https://linux.die.net/man/2/send and https://github.com/deepfryed/beanstalk-client/issues/32
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0x0    // Define it as 0 so it's not requested in systems that don't support the SIGPIPE error suppression
#endif 

/**
 * Describe de forma humana un sockaddr:
 *
 * @param buff     el buffer de escritura
 * @param buffsize el tamaño del buffer  de escritura
 *
 * @param af    address family
 * @param addr  la dirección en si
 * @param nport puerto en network byte order
 *
 */
const char *
sockaddr_to_human(char *buff, const size_t buffsize,
                  const struct sockaddr *addr);



/**
 * Escribe n bytes de buff en fd de forma bloqueante
 *
 * Retorna 0 si se realizó sin problema y errno si hubo problemas
 */
int
sock_blocking_write(const int fd, buffer *b);


/**
 * copia todo el contenido de source a dest de forma bloqueante.
 *
 * Retorna 0 si se realizó sin problema y errno si hubo problemas
 */
int
sock_blocking_copy(const int source, const int dest);

#endif
