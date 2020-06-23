#include "sctpnio/sctpnio.h"

#include "sctpnio/sctpnio_users.h"
#include "sctpnio/sctpnio_configs.h"
#include "sctpnio/sctpnio_metrics.h"

/** Gets the struct (sctp *) from the selector key */
/* We have to include this line in all the sctpnio/ classes because it collides with the other definition of attachment otherwise*/
#define ATTACHMENT(key) ((struct sctp *)(key)->data)

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
static void sctp_read(struct selector_key *key);
static void sctp_write(struct selector_key *key);
static void sctp_close(struct selector_key *key);
static void sctp_done(struct selector_key *key);

/** Handler for when the TYPE + CMD pair is invalid, responds and closes connection */
static void handle_invalid_type_cmd(struct selector_key *key);

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

    switch (d->info.type)
    {
    case TYPE_USERS:
        switch (d->info.cmd)
        {
        case CMD_LIST:
            n = send_user_list(key);
            break;
        case CMD_CREATE:
        {
            n = send_create_user(key);
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
            n = send_list_configs(key);
            break;
        }
        case CMD_EDIT:
        {
            n = send_edit_config(key);
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
            n = send_list_metrics(key);
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
                n = send_login_user(key);
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
    // struct sockaddr from;
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
    ssize_t length = sctp_recvmsg(d->client_fd, ptr, count, NULL, 0, &sender_info, &flags);
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
                ret = handle_user_list(key);
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
