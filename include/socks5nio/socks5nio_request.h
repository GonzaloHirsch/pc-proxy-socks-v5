#ifndef __SOCKS5NIO_REQUEST_H__
#define __SOCKS5NIO_REQUEST_H__

#include "socks5nio/socks5nio.h"
#include "socks5nio/socks5nio_error.h"

void
request_init(const unsigned state, struct selector_key *key);
void
request_close(const unsigned state, struct selector_key *key);
unsigned
request_read(struct selector_key *key);


#endif