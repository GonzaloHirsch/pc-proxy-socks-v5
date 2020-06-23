#ifndef __SOCKS5NIO_HELLO_H__
#define __SOCKS5NIO_HELLO_H__

#include "socks5nio/socks5nio.h"

void
hello_read_init(const unsigned state, struct selector_key *key);
void
hello_read_close(const unsigned state, struct selector_key *key);
unsigned
hello_read(struct selector_key *key);
void
hello_write_init(const unsigned state, struct selector_key *key);
void
hello_write_close(const unsigned state, struct selector_key *key);
unsigned
hello_write(struct selector_key *key);

#endif