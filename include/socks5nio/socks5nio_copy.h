#ifndef __SOCKS5NIO_COPY_H__
#define __SOCKS5NIO_COPY_H__

#include "socks5nio/socks5nio.h"

void
copy_init(const unsigned state, struct selector_key *key);
unsigned
copy_read(struct selector_key *key);
unsigned
copy_write(struct selector_key *key);

#endif