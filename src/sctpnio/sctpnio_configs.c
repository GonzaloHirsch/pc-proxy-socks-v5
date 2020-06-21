#include "sctpnio/sctpnio_configs.h"

/** Gets the struct (sctp *) from the selector key */
#define ATTACHMENT(key) ((struct sctp *)(key)->data)

ssize_t send_list_configs(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);
    d->datagram.configs_list.configs_data[2] = d->error;
    ssize_t n = sctp_sendmsg(key->fd, (void *)d->datagram.configs_list.configs_data, d->datagram.configs_list.configs_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    free(d->datagram.configs_list.configs_data);
    return n;
}

ssize_t send_edit_config(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);
    uint8_t config_data[4] = {d->info.type, d->info.cmd, d->error, d->datagram.config_edit.config_type};
    ssize_t n = sctp_sendmsg(key->fd, (void *)config_data, N(config_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    return n;
}

unsigned handle_list_configs(struct selector_key *key)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);

    uint8_t data[11] = {0};
    data[0] = d->info.type;
    data[1] = d->info.cmd;
    data[2] = 0x00; // Status
    data[3] = 0x04; // Amount of configs to send
    hton16(data + 4, options->socks5_buffer_size);
    hton16(data + 4 + 2, options->sctp_buffer_size);
    hton16(data + 4 + 4, options->doh.buffer_size);
    data[10] = options->timeout;

    // Allocating and copying
    d->datagram.configs_list.configs_data = malloc(N(data));
    memcpy(d->datagram.configs_list.configs_data, data, N(data));

    d->datagram.configs_list.configs_len = N(data);

    return SCTP_RESPONSE;
}

unsigned handle_config_edit(struct selector_key *key, buffer *b)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);

    // Variable for the config type
    uint8_t config_type;

    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        config_type = *b->read;
        d->error = SCTP_ERROR_NO_ERROR;
    }
    else
    {
        d->error = SCTP_ERROR_INVALID_TYPE;
        return SCTP_ERROR;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    bool parseOk = false;

    // In the switch extract the number and set it in the cofig
    switch (config_type)
    {
    case CONF_SOCKS5_BUFF:
        d->datagram.config_edit.config_type = CONF_SOCKS5_BUFF;
        parseOk = get_16_bit_integer_from_buffer(b, &d->datagram.config_edit.value.val16);
        if (parseOk)
        {
            options->socks5_buffer_size = d->datagram.config_edit.value.val16;
        }
        break;
    case CONF_SCTP_BUFF:
        d->datagram.config_edit.config_type = CONF_SCTP_BUFF;
        parseOk = get_16_bit_integer_from_buffer(b, &d->datagram.config_edit.value.val16);
        if (parseOk)
        {
            printf("My new val is %u\n", d->datagram.config_edit.value.val16);
            options->sctp_buffer_size = d->datagram.config_edit.value.val16;
        }
        break;
    case CONF_DOH_BUFF:
        d->datagram.config_edit.config_type = CONF_DOH_BUFF;
        parseOk = get_16_bit_integer_from_buffer(b, &d->datagram.config_edit.value.val16);
        if (parseOk)
        {
            options->doh.buffer_size = d->datagram.config_edit.value.val16;
        }
        break;
    case CONF_TIMEOUT:
        d->datagram.config_edit.config_type = CONF_TIMEOUT;
        parseOk = get_8_bit_integer_from_buffer(b, &d->datagram.config_edit.value.val8);
        if (parseOk)
        {
            options->timeout = d->datagram.config_edit.value.val8;
        }
        break;
    default:
        return SCTP_ERROR;
    }

    if (!parseOk)
    {
        d->error = SCTP_ERROR_INVALID_VALUE;
        return SCTP_ERROR;
    }
    else
    {
        d->error = SCTP_ERROR_NO_ERROR;
    }

    return SCTP_RESPONSE;
}