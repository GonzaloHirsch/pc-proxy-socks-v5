#ifndef __SOCKS5NIO_USERPASS_H__
#define __SOCKS5NIO_USERPASS_H__

#include "socks5nio/socks5nio.h"

void
userpass_read_init(const unsigned state, struct selector_key *key);
void
userpass_read_close(const unsigned state, struct selector_key *key);
unsigned
userpass_read(struct selector_key *key);
void
userpass_write_init(const unsigned state, struct selector_key *key);
void
userpass_write_close(const unsigned state, struct selector_key *key);
unsigned
userpass_write(struct selector_key *key);


#endif