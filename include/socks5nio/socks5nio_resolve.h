#ifndef __SOCKS5NIO_RESOLVE_H__
#define __SOCKS5NIO_RESOLVE_H__

#include "socks5nio/socks5nio.h"
#include "socks5nio/socks5nio_error.h"

void
resolve_init(const unsigned state, struct selector_key *key);
void
resolve_close(const unsigned state, struct selector_key *key);
unsigned
resolve_read(struct selector_key *key);
unsigned
resolve_write(struct selector_key *key);

#endif