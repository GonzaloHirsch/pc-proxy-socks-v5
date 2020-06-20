#ifndef __SOCKS5NIO_ERROR_H__
#define __SOCKS5NIO_ERROR_H__

#include "socks5nio/socks5nio.h"
#include "logging.h"

void
send_reply_failure(struct selector_key * key);
void determine_connect_error(int error);

#endif