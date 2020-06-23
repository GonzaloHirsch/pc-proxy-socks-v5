#include "socks5nio/socks5nio_error.h"

void
send_reply_failure(struct selector_key * key)
{
    struct socks5 *s = ATTACHMENT(key);

    //Reply to be send
    
    // Build the reply
    uint8_t reply_s = 7;
    uint8_t *reply = malloc(reply_s);
    reply[0] = 0x05;            // Version
    reply[1] = s->reply_type != -1 ? s->reply_type : REPLY_RESP_GENERAL_FAILURE;  // Reply field
    reply[2] = 0x00;            //Rsv
    reply[3] = 0x00;
    reply[4] = 0x00;
    reply[5] = 0x00;
    reply[6] = 0x00;

    ssize_t n = send(s->client_fd, reply , reply_s, MSG_DONTWAIT);

    // Logging the error in the request
    log_request(s->reply_type != -1 ? s->reply_type : REPLY_RESP_GENERAL_FAILURE, s->username, (const struct sockaddr *)&s->client_addr, (const struct sockaddr *)&s->origin_info.origin_addr, s->origin_info.resolve_addr);

    // Metrics
    add_transfered_bytes(n);
    free(reply);
}

