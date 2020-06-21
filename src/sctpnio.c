#include "sctpnio.h"
#define N(x) (sizeof(x) / sizeof((x)[0]))

/*
    Pool for the reusing of instances
*/
/** Max amount of items in pool */
//static const unsigned max_pool = 50;
/** Actual pool size */
//static unsigned pool_size = 0;
/** Actual pool of objects */
static struct sctp *pool = 0;

// Handler function declarations
static unsigned handle_request(struct selector_key *key);
static unsigned handle_normal_request(struct selector_key *key);
static unsigned handle_login_request(struct selector_key *key);
static void sctp_read(struct selector_key *key);
static void sctp_write(struct selector_key *key);
static void sctp_close(struct selector_key *key);
static void sctp_done(struct selector_key *key);

// TYPE + CMD handlers
static unsigned handle_list_users(struct selector_key *key);
static unsigned handle_list_configs(struct selector_key *key);
static unsigned handle_user_create(struct selector_key *key, buffer *b);
static unsigned handle_list_metrics(struct selector_key *key);
static unsigned handle_config_edit(struct selector_key *key, buffer *b);

/** Handler for when the TYPE + CMD pair is invalid, responds and closes connection */
static void handle_invalid_type_cmd(struct selector_key *key);

// Response preparers
static uint8_t *prepare_list_users(uint8_t **users, int count, int len);

// Helpers
static bool get_8_bit_integer_from_buffer(buffer *b, uint8_t *n);
static bool get_16_bit_integer_from_buffer(buffer *b, uint16_t *n);
static void free_list_users(uint8_t **users, int count);

// Selector event handler
static const struct fd_handler sctp_handler = {
    .handle_read = sctp_read,
    .handle_write = sctp_write,
    .handle_close = sctp_close,
    .handle_block = NULL,
};

///////////////////////////////////////////////////////////////////
// OBJECT POOL
///////////////////////////////////////////////////////////////////

/** Actual destroying */
static void
sctp_destroy_(struct sctp *s)
{
    free(s->buffer_read.data);
    free(s->buffer_write.data);
    free(s);
}

/**
 * Destroys a "struct sctp", takes into account the references
 * and the object pool
 */
/*
static void
sctp_destroy(struct sctp *s)
{
    if (s == NULL)
    {
        // Nothing to do
    }
    else if (s->references == 1)
    {
        if (s != NULL)
        {
            if (pool_size < max_pool)
            {
                s->next = pool;
                pool = s;
                pool_size++;
            }
            else
            {
                sctp_destroy_(s);
            }
        }
    }
    else
    {
        s->references -= 1;
    }
}
*/

void sctp_pool_destroy(void)
{
    struct sctp *next, *s;
    for (s = pool; s != NULL; s = next)
    {
        next = s->next;
        free(s);
    }
}

///////////////////////////////////////////////////////////////////
// CONNECTION
///////////////////////////////////////////////////////////////////

/** 
 * Creation of a new sctp struct
 */
static struct sctp *sctp_new(const int client)
{
    // Initialize the Socks5 structure which contain the state machine and other info for the socket.
    struct sctp *sctpState = malloc(sizeof(struct sctp));
    if (sctpState == NULL)
    {
        perror("Error: Initizalizing null Sctp\n");
    }

    // Write Buffer for the socket(Initialized)
    buffer_init(&(sctpState->buffer_write), options->sctp_buffer_size + 1, malloc(options->sctp_buffer_size + 1));
    // Read Buffer for the socket(Initialized)
    buffer_init(&(sctpState->buffer_read), options->sctp_buffer_size + 1, malloc(options->sctp_buffer_size + 1));

    // Setting initial values to avoid errors
    sctpState->is_logged = 0;
    sctpState->state = SCTP_REQUEST;
    sctpState->error = SCTP_ERROR_NO_ERROR;
    sctpState->references = 1;

    // Intialize the client_fd
    sctpState->client_fd = client;

    return sctpState;
}

/**
 * Accepting a new passive connection
 * */
void sctp_passive_accept(struct selector_key *key)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct sctp *state = NULL;

    // Accepting the new connection
    const int client = accept(key->fd, (struct sockaddr *)&client_addr, &client_addr_len);

    // Checking if there was a fd given
    if (client == -1)
    {
        goto fail;
    }

    // Trying to set the new fd in the selector
    if (selector_fd_set_nio(client) == -1)
    {
        goto fail;
    }

    state = sctp_new(client);
    if (state == NULL)
    {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }

    // Setting the client information in the struct
    memcpy(&state->client_addr, &client_addr, client_addr_len);
    state->client_addr_len = client_addr_len;

    // Adding to metrics
    add_current_connections(1);
    add_historic_connections(1);

    // Signal to avoid the SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

    if (SELECTOR_SUCCESS != selector_register(key->s, client, &sctp_handler, OP_READ, state))
    {
        goto fail;
    }
    return;
fail:
    if (client != -1)
    {
        close(client);
    }

    sctp_destroy_(state);
}

///////////////////////////////////////////////////////////////////
// HANDLERS
///////////////////////////////////////////////////////////////////

/** Gets the struct (sctp *) from the selector key */
#define ATTACHMENT(key) ((struct sctp *)(key)->data)

static void
sctp_read(struct selector_key *key)
{
    // Result, next state to move in the process
    unsigned ret = handle_request(key);

    if (ret == SCTP_FATAL_ERROR)
    {
        sctp_done(key);
    }
    else
    {
        ATTACHMENT(key)->state = ret;
    }
}

static void
sctp_write(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);

    ssize_t n;
    size_t nr;
    buffer_read_ptr(&d->buffer_write, &nr);

    switch (d->info.type)
    {
    case TYPE_USERS:
        switch (d->info.cmd)
        {
        case CMD_LIST:
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
                uint8_t user_list_data[4 + packet_len];
                user_list_data[0] = d->info.type;
                user_list_data[1] = d->info.cmd;
                user_list_data[2] = d->error;
                user_list_data[3] = d->datagram.user_list.user_count;
                memcpy(&user_list_data[4], users, packet_len);
                n = sctp_sendmsg(key->fd, (void *)user_list_data, 4 + packet_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
                free(users);
                free_list_users(d->datagram.user_list.users, d->datagram.user_list.user_count);
            }
            break;
        case CMD_CREATE:
        {
            uint8_t user_create_data[] = {d->info.type, d->info.cmd, d->error, SCTP_VERSION};
            n = sctp_sendmsg(key->fd, (void *)user_create_data, N(user_create_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            break;
        }
        default:
            handle_invalid_type_cmd(key);
            return;
        }
        break;
    case TYPE_CONFIG:
        switch (d->info.cmd)
        {
        case CMD_LIST:
        {
            d->datagram.configs_list.configs_data[2] = d->error;
            n = sctp_sendmsg(key->fd, (void *)d->datagram.configs_list.configs_data, d->datagram.configs_list.configs_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            free(d->datagram.configs_list.configs_data);
            break;
        }
        case CMD_EDIT:
        {
            uint8_t config_data[4] = {d->info.type, d->info.cmd, d->error, d->datagram.config_edit.config_type};
            n = sctp_sendmsg(key->fd, (void *)config_data, N(config_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            break;
        }
        default:
            handle_invalid_type_cmd(key);
            return;
        }
        break;
    case TYPE_METRICS:
        switch (d->info.cmd)
        {
        case CMD_LIST:
        {
            d->datagram.metrics_list.metrics_data[2] = d->error;
            n = sctp_sendmsg(key->fd, (void *)d->datagram.metrics_list.metrics_data, d->datagram.metrics_list.metrics_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            free(d->datagram.metrics_list.metrics_data);
            break;
        }
        default:
            handle_invalid_type_cmd(key);
            return;
        }
        break;
    // TYPE not allowed
    case TYPE_NOTYPE:
        switch (d->info.cmd)
        {
        case CMD_NOCMD:
        {
            if (d->error == SCTP_ERROR_INVALID_CMD || d->error == SCTP_ERROR_INVALID_TYPE)
            {
                handle_invalid_type_cmd(key);
                return;
            }
            else
            {
                uint8_t login_data[] = {SCTP_VERSION, d->error};
                n = sctp_sendmsg(key->fd, (void *)login_data, N(login_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            }
            break;
        }
        default:
            handle_invalid_type_cmd(key);
            return;
        }
        break;
    default:
        handle_invalid_type_cmd(key);
        return;
    }

    if (n > 0)
    {
        // Add bytes to metrics
        add_transfered_bytes(n);

        // Resetting the error
        d->error = SCTP_ERROR_NO_ERROR;
        d->state = SCTP_REQUEST;

        // Setting the fd to read.
        if (SELECTOR_SUCCESS != selector_set_interest(key->s, key->fd, OP_READ))
        {
            sctp_done(key);
        }
    }
    else
    {
        sctp_done(key);
    }
}

static void
sctp_close(struct selector_key *key)
{
    // Removing the current connection from the metrics
    remove_current_connections(1);

    // Destroying the struct, calling the destroy directly because the pool is not implemented
    sctp_destroy_(ATTACHMENT(key));
}

static void
sctp_done(struct selector_key *key)
{
    int client_fd = ATTACHMENT(key)->client_fd;
    selector_status ss = selector_unregister_fd(key->s, ATTACHMENT(key)->client_fd);
    if (ss != SELECTOR_SUCCESS)
    {
        printf("SCTP is done for %d\n", client_fd);
        abort();
    }
    close(client_fd);
}

///////////////////////////////////////////////////////////////////
// REQUESTS
///////////////////////////////////////////////////////////////////

static bool determine_type(int val, TYPE *t)
{
    switch (val)
    {
    case TYPE_NOTYPE:
        *t = TYPE_NOTYPE;
        return true;
    case TYPE_CONFIG:
        *t = TYPE_CONFIG;
        return true;
    case TYPE_METRICS:
        *t = TYPE_METRICS;
        return true;
    case TYPE_USERS:
        *t = TYPE_USERS;
        return true;
    default:
        return false;
    }
}

static bool determine_cmd(int val, CMD *c)
{
    switch (val)
    {
    case CMD_CREATE:
        *c = CMD_CREATE;
        return true;
    case CMD_EDIT:
        *c = CMD_EDIT;
        return true;
    case CMD_LIST:
        *c = CMD_LIST;
        return true;
    case CMD_NOCMD:
        *c = CMD_NOCMD;
        return true;
    default:
        return false;
    }
}

static void handle_invalid_type_cmd(struct selector_key *key)
{
    uint8_t data[3] = {0x00, 0x00, ATTACHMENT(key)->error};
    sctp_sendmsg(key->fd, (void *)data, N(data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    sctp_done(key);
}

static unsigned handle_request(struct selector_key *key)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);
    // Struct for information about the sender
    struct sctp_sndrcvinfo sender_info;
    // Variable for the from data
    struct sockaddr from;
    // Getting the buffer to read the data
    buffer *b = &d->buffer_read;
    // Amount that can be read
    size_t count;
    // Flags for the recv
    int flags = 0;
    // Getting the pointer to reading data
    uint8_t *ptr = buffer_write_ptr(b, &count);
    // Variable for the return value of the request
    unsigned ret = SCTP_RESPONSE;

    // Receiving the message
    ssize_t length = sctp_recvmsg(d->client_fd, ptr, count, &from, 0, &sender_info, &flags);
    if (length <= 0)
    {
        return SCTP_FATAL_ERROR;
    }

    // Add bytes to metrics
    add_transfered_bytes(length);

    // Advancing the buffer
    buffer_write_adv(b, length);

    // If user is logged, parse a request
    if (d->is_logged)
    {
        ret = handle_normal_request(key);
    }
    else
    {
        ret = handle_login_request(key);
    }

    return ret;
}

static unsigned handle_normal_request(struct selector_key *key)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);
    // Getting the buffer to read the data
    buffer *b = &d->buffer_read;
    // Variable for the return value of the request
    unsigned ret = SCTP_RESPONSE;
    // Values that determine the request type parser
    int given_type = -1, given_cmd = -1;

    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        given_type = *b->read;
    }
    else
    {
        d->error = SCTP_ERROR_INVALID_TYPE;
        ret = SCTP_ERROR;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    // reading the request CMD
    if (buffer_can_read(b))
    {
        given_cmd = *b->read;
    }
    else
    {
        d->error = SCTP_ERROR_INVALID_CMD;
        ret = SCTP_ERROR;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    bool type_ok = determine_type(given_type, &d->info.type);
    bool cmd_ok = determine_cmd(given_cmd, &d->info.cmd);

    if (!type_ok)
    {
        d->error = SCTP_ERROR_INVALID_TYPE;
        ret = SCTP_ERROR;
    }
    if (!cmd_ok)
    {
        d->error = SCTP_ERROR_INVALID_CMD;
        ret = SCTP_ERROR;
    }

    if (ret != SCTP_ERROR)
    {
        // Switching TYPE and CMD to determine available operations
        switch (d->info.type)
        {
        case TYPE_USERS:
            switch (d->info.cmd)
            {
            case CMD_LIST:
                ret = handle_list_users(key);
                break;
            case CMD_CREATE:
                ret = handle_user_create(key, b);
                break;
            default: // CMD not allowed
                ret = SCTP_ERROR;
                break;
            }
            break;
        case TYPE_CONFIG:
            switch (d->info.cmd)
            {
            case CMD_LIST:
                ret = handle_list_configs(key);
                break;
            case CMD_EDIT:
                ret = handle_config_edit(key, b);
                break;
            default: // CMD not allowed
                ret = SCTP_ERROR;
                break;
            }
            break;
        case TYPE_METRICS:
            switch (d->info.cmd)
            {
            case CMD_LIST:
                ret = handle_list_metrics(key);
                break;
            default: // CMD not allowed
                ret = SCTP_ERROR;
                break;
            }
            break;
        case TYPE_NOTYPE: // TYPE not allowed
            ret = SCTP_ERROR;
            break;
        }
    }

    // Compacting the buffer
    buffer_compact(b);

    if (SELECTOR_SUCCESS != selector_set_interest(key->s, key->fd, OP_WRITE))
    {
        ret = SCTP_FATAL_ERROR;
    }

    return ret;
}

static unsigned handle_login_request(struct selector_key *key)
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

static unsigned handle_list_users(struct selector_key *key)
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

static unsigned handle_user_create(struct selector_key *key, buffer *b)
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

static unsigned handle_list_metrics(struct selector_key *key)
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

static unsigned handle_list_configs(struct selector_key *key)
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

static unsigned handle_config_edit(struct selector_key *key, buffer *b)
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

///////////////////////////////////////////////////////////////////
// RESPONSE PREPARERS
///////////////////////////////////////////////////////////////////

static uint8_t *prepare_list_users(uint8_t **users, int count, int len)
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

static void free_list_users(uint8_t **users, int count)
{
    free_list_user_admin(users, count);
}

///////////////////////////////////////////////////////////////////
// HELPERS
///////////////////////////////////////////////////////////////////

static bool get_16_bit_integer_from_buffer(buffer *b, uint16_t *n)
{
    uint8_t num[2];

    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        num[0] = *b->read;
    }
    else
    {
        return false;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        num[1] = *b->read;
    }
    else
    {
        return false;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    *n = ntoh16(num);

    return true;
}

static bool get_8_bit_integer_from_buffer(buffer *b, uint8_t *n)
{
    // Reading the request TYPE
    if (buffer_can_read(b))
    {
        *n = *b->read;
    }
    else
    {
        return false;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    return true;
}