#include "sctpnio/sctpnio_configs.h"

#define SCTP_BUFFER_MIN 256
#define SCTP_BUFFER_MAX 8192

#define SOCKS5_BUFFER_MIN 256
#define SOCKS5_BUFFER_MAX 32768

#define DOH_BUFFER_MIN 256
#define DOH_BUFFER_MAX 4096

#define TIMEOUT_MIN 5
#define TIMEOUT_MAX 60

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
    data[10] = options->disectors_enabled == true ? 0x01 : 0x00;

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

            if (d->datagram.config_edit.value.val16 >= SOCKS5_BUFFER_MIN && d->datagram.config_edit.value.val16 <= SOCKS5_BUFFER_MAX)
            {
                options->socks5_buffer_size = d->datagram.config_edit.value.val16;
            }
            else
            {
                parseOk = false;
            }
        }
        break;
    case CONF_SCTP_BUFF:
        d->datagram.config_edit.config_type = CONF_SCTP_BUFF;
        parseOk = get_16_bit_integer_from_buffer(b, &d->datagram.config_edit.value.val16);
        if (parseOk)
        {
            if (d->datagram.config_edit.value.val16 >= SCTP_BUFFER_MIN && d->datagram.config_edit.value.val16 <= SCTP_BUFFER_MAX)
            {
                options->sctp_buffer_size = d->datagram.config_edit.value.val16;
            }
            else
            {
                parseOk = false;
            }
        }
        break;
    case CONF_DOH_BUFF:
        d->datagram.config_edit.config_type = CONF_DOH_BUFF;
        parseOk = get_16_bit_integer_from_buffer(b, &d->datagram.config_edit.value.val16);
        if (parseOk)
        {
            if (d->datagram.config_edit.value.val16 >= DOH_BUFFER_MIN && d->datagram.config_edit.value.val16 <= DOH_BUFFER_MAX)
            {
                options->doh.buffer_size = d->datagram.config_edit.value.val16;
            }
            else
            {
                parseOk = false;
            }
        }
        break;
    case CONF_DISECTORS:
        d->datagram.config_edit.config_type = CONF_DISECTORS;
        parseOk = get_8_bit_integer_from_buffer(b, &d->datagram.config_edit.value.val8);
        if (parseOk)
        {
            if (d->datagram.config_edit.value.val8 > 0)
            {
                options->disectors_enabled = true;
            }
            else
            {
                options->disectors_enabled = false;
            }
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