#ifndef __SCTPNIO_USERS_H__
#define __SCTPNIO_USERS_H__

#include <stdint.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "selector.h"
#include "sctpnio.h"
#include "io_utils/buffer.h"
#include "authentication.h"

ssize_t send_create_user(struct selector_key *key);

ssize_t send_login_user(struct selector_key *key);

ssize_t send_user_list(struct selector_key *key);

unsigned handle_user_list(struct selector_key *key);

unsigned handle_login_request(struct selector_key *key);

unsigned handle_user_create(struct selector_key *key, buffer *b);

#endif