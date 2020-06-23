#ifndef __SCTPNIO_CONFIGS_H__
#define __SCTPNIO_CONFIGS_H__

#include <stdint.h>
#include <unistd.h>
#include "sctpnio/sctpnio.h"
#include "io_utils/buffer_utils.h"
#include "io_utils/buffer.h"
#include "args.h"
#include "selector.h"

ssize_t send_list_configs(struct selector_key *key);

ssize_t send_edit_config(struct selector_key *key);

unsigned handle_list_configs(struct selector_key *key);

unsigned handle_config_edit(struct selector_key *key, buffer *b);

#endif