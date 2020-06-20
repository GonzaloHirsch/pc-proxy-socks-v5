#ifndef __SOCKS_5_NIO_CONNECTING_H__
#define __SOCKS_5_NIO_CONNECTING_H__

#include "socks5nio/socks5nio.h"
#include "socks5nio/socks5nio_error.h"
#include "logging.h"

void connecting_init(const unsigned state, struct selector_key *key);
unsigned connecting_read(struct selector_key * key);
unsigned connecting_write(struct selector_key *key);
void connecting_close(const unsigned state, struct selector_key *key);

#endif