#include "sctpnio/sctpnio_users.h"

/** Gets the struct (sctp *) from the selector key */
#define ATTACHMENT(key) ((struct sctp *)(key)->data)

uint8_t *prepare_list_users(uint8_t **users, int count, int len);
void free_list_users(uint8_t **users, int count);

ssize_t send_create_user(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);

    uint8_t user_create_data[] = {d->info.type, d->info.cmd, d->error, SCTP_VERSION};
    ssize_t n = sctp_sendmsg(key->fd, (void *)user_create_data, N(user_create_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);

    return n;
}

ssize_t send_login_user(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);
    uint8_t login_data[] = {SCTP_VERSION, d->error};
    ssize_t n = sctp_sendmsg(key->fd, (void *)login_data, N(login_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    return n;
}

unsigned handle_user_create(struct selector_key *key, buffer *b)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);
    // Boolean for parser error and auth validity
    bool error = false;
    // Variable for the return value of the request
    unsigned ret = SCTP_RESPONSE;

    // Initializing the u+p parser
    up_req_parser_init(&d->paser.up_request);

    // Parsing the information
    const enum up_req_state st = up_consume_message(b, &d->paser.up_request, &error);
    // Checking if the parser is done parsing the message
    if (up_done_parsing(st, &error) && !error)
    {
        selector_status ss = selector_set_interest(key->s, key->fd, OP_WRITE);
        if (ss != SELECTOR_SUCCESS)
        {
            ret = SCTP_FATAL_ERROR;
        }
        else
        {
            d->datagram.user.user = d->paser.up_request.uid;
            d->datagram.user.pass = d->paser.up_request.pw;
            memcpy(d->datagram.user.user, d->paser.up_request.uid, d->paser.up_request.uidLen);
            memcpy(d->datagram.user.pass, d->paser.up_request.pw, d->paser.up_request.pwLen);
            d->datagram.user.ulen = d->paser.up_request.uidLen;
            d->datagram.user.plen = d->paser.up_request.pwLen;

            // Validating the login request
            error = !create_user_admin(d->datagram.user.user, d->datagram.user.pass, d->datagram.user.ulen, d->datagram.user.plen);

            if (error)
            {
                d->error = SCTP_ERROR_INVALID_DATA;
                ret = SCTP_ERROR;
            }
            else
            {
                d->error = SCTP_ERROR_NO_ERROR;
            }
        }
    }

    // Liberating the parser
    free_up_req_parser(&d->paser.up_request);

    return ret;
}

ssize_t send_user_list(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);
    ssize_t n;

    if (d->state == SCTP_ERROR)
    {
        uint8_t user_list_error_data[] = {d->info.type, d->info.cmd, d->error, 0};
        n = sctp_sendmsg(key->fd, (void *)user_list_error_data, N(user_list_error_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    }
    else
    {
        int packet_len = d->datagram.user_list.users_len + d->datagram.user_list.user_count - 1;
        // Getting the representation for the users
        uint8_t *users = prepare_list_users(d->datagram.user_list.users, d->datagram.user_list.user_count, d->datagram.user_list.users_len);
        // Adding the info to the buffer
        uint8_t user_list_data[7 + packet_len];
        user_list_data[0] = d->info.type;
        user_list_data[1] = d->info.cmd;
        user_list_data[2] = d->error;
        hton32(user_list_data + 3, (uint32_t)d->datagram.user_list.user_count);
        memcpy(&user_list_data[7], users, packet_len);
        n = sctp_sendmsg(key->fd, (void *)user_list_data, 7 + packet_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
        free(users);
        free_list_users(d->datagram.user_list.users, d->datagram.user_list.user_count);
    }

    return n;
}

unsigned handle_user_list(struct selector_key *key)
{
    // Getting all the users
    uint8_t count = 0;
    int size = 0;
    list_user_admin(ATTACHMENT(key)->datagram.user_list.users, &count, &size);
    if (ATTACHMENT(key)->datagram.user_list.users == NULL)
    {
        // Setting general error
        ATTACHMENT(key)->error = SCTP_ERROR_GENERAL_ERROR;
        return SCTP_ERROR;
    }

    // Adding the list and the count to the requets data
    ATTACHMENT(key)->datagram.user_list.user_count = count;
    ATTACHMENT(key)->datagram.user_list.users_len = size;

    return SCTP_RESPONSE;
}

uint8_t *prepare_list_users(uint8_t **users, int count, int len)
{
    int i = 0, previous_len = 0, slen = 0;
    uint8_t *value = calloc(len + count - 1, sizeof(uint8_t)); // Allocating for all users + the sepparator between them
    if (value == NULL)
    {
        return NULL;
    }

    for (i = 0; i < count; i++)
    {
        slen = strlen((const char *)users[i]);
        if (i == 0)
        {
            memcpy(value, users[i], slen);
            previous_len = slen;
        }
        else
        {
            value[previous_len] = 0x00;
            memcpy(value + previous_len + 1, users[i], slen);
            previous_len += 1 + slen;
        }
    }
    return value;
}

void free_list_users(uint8_t **users, int count)
{
    free_list_user_admin(users, count);
}

unsigned handle_login_request(struct selector_key *key)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);
    // Getting the buffer to read the data
    buffer *b = &d->buffer_read;
    // Boolean for parser error and auth validity
    bool error = false, auth_valid = false;
    // Variable for the return value of the request
    unsigned ret = SCTP_RESPONSE;

    // Initializing the u+p parser
    up_req_parser_init(&d->paser.up_request);

    // Parsing the information
    const enum up_req_state st = up_consume_message(b, &d->paser.up_request, &error);
    // Checking if the parser is done parsing the message
    if (up_done_parsing(st, &error) && !error)
    {
        selector_status ss = selector_set_interest(key->s, key->fd, OP_WRITE);
        if (ss != SELECTOR_SUCCESS)
        {
            ret = SCTP_FATAL_ERROR;
        }
        else
        {
            uint8_t *uid = d->paser.up_request.uid;
            uint8_t *pw = d->paser.up_request.pw;
            uint8_t uid_l = d->paser.up_request.uidLen;

            // Validating the login request
            auth_valid = validate_user_admin(uid, pw);

            if (auth_valid)
            {
                // Setting the username in the sctp object
                memset(d->username, 0, 255);
                memcpy(d->username, uid, uid_l);
                d->is_logged = true;
                d->error = SCTP_ERROR_NO_ERROR;
                ret = SCTP_RESPONSE;
            }
            else
            {
                // Setting error for invalid data sent
                d->is_logged = false;
                d->error = SCTP_ERROR_INVALID_DATA;
                ret = SCTP_ERROR;
            }

            // Setting the type and command as no type and no command
            d->info.cmd = CMD_NOCMD;
            d->info.type = TYPE_NOTYPE;
        }
    }

    // Liberating the parser
    free_up_req_parser(&d->paser.up_request);

    return ret;
}