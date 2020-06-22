#ifndef __SOCKS5NIO_COPY_H__
#define __SOCKS5NIO_COPY_H__

#include "socks5nio/socks5nio.h"
#include "./io_utils/encoding.h"
#include "logging.h"
#include "args.h"

void
copy_init(const unsigned state, struct selector_key *key);
void
copy_close(const unsigned state, struct selector_key *key);
unsigned
copy_read(struct selector_key *key);
unsigned
copy_write(struct selector_key *key);

#endif