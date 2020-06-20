#include "sctpnio/sctpnio_metrics.h"

/** Gets the struct (sctp *) from the selector key */
#define ATTACHMENT(key) ((struct sctp *)(key)->data)

ssize_t send_list_metrics(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);
    d->datagram.metrics_list.metrics_data[2] = d->error;
    ssize_t n = sctp_sendmsg(key->fd, (void *)d->datagram.metrics_list.metrics_data, d->datagram.metrics_list.metrics_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    free(d->datagram.metrics_list.metrics_data);
    return n;
}

unsigned handle_list_metrics(struct selector_key *key)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);

    // Getting the metrics
    metrics m = get_metrics();

    uint8_t data[20] = {0};
    data[0] = d->info.type;
    data[1] = d->info.cmd;
    data[2] = 0x00;
    data[3] = m->metrics_count;
    hton64(data + 4, m->transfered_bytes);
    hton32(data + 4 + 8, m->historic_connections);
    hton32(data + 4 + 8 + 4, m->current_connections);

    // Allocating and copying
    d->datagram.metrics_list.metrics_data = malloc(N(data));
    memcpy(d->datagram.metrics_list.metrics_data, data, N(data));

    d->datagram.metrics_list.metrics_len = N(data);

    return SCTP_RESPONSE;
}