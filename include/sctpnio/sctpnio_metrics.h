#ifndef __SCTPNIO_METRICS_H__
#define __SCTPNIO_METRICS_H__

#include <unistd.h>
#include <stdint.h>
#include "sctpnio/sctpnio.h"
#include "io_utils/buffer_utils.h"
#include "io_utils/buffer.h"
#include "metrics.h"
#include "selector.h"

ssize_t send_list_metrics(struct selector_key *key);

unsigned handle_list_metrics(struct selector_key *key);

#endif