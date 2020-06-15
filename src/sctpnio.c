#include "sctpnio.h"
#define N(x) (sizeof(x) / sizeof((x)[0]))

/*
    Pool for the reusing of instances
*/
/** Max amount of items in pool */
static const unsigned max_pool = 50;
/** Actual pool size */
static unsigned pool_size = 0;
/** Actual pool of objects */
static struct sctp *pool = 0;
/** Selector key for the SIGPIPE handler */
static struct selector_key *current_key;

// Handler function declarations
static unsigned handle_request(struct selector_key *key);
static unsigned handle_normal_request(struct selector_key *key);
static unsigned handle_login_request(struct selector_key *key);
static void sctp_read(struct selector_key *key);
static void sctp_write(struct selector_key *key);
static void sctp_close(struct selector_key *key);

// Selector event handler
static const struct fd_handler sctp_handler = {
    .handle_read = sctp_read,
    .handle_write = sctp_write,
    .handle_close = sctp_close,
    .handle_block = NULL,
};

/** Handler for the SIGPIPE signal */
static void sctp_sigpipe_handler()
{
    if (selector_set_interest(current_key->s, current_key->fd, OP_NOOP) != SELECTOR_SUCCESS)
    {
        exit(0);
    }
    else
    {
        exit(0);
    }
}

///////////////////////////////////////////////////////////////////
// OBJECT POOL
///////////////////////////////////////////////////////////////////

/** Actual destroying */
static void
sctp_destroy_(struct sctp *s)
{
    free(s);
}

/**
 * Destroys a "struct sctp", takes into account the references
 * and the object pool
 */
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
    buffer_init(&(sctpState->buffer_write), SCTP_BUFFERSIZE + 1, malloc(SCTP_BUFFERSIZE + 1));
    // Read Buffer for the socket(Initialized)
    buffer_init(&(sctpState->buffer_read), SCTP_BUFFERSIZE + 1, malloc(SCTP_BUFFERSIZE + 1));

    // Intialize the client_fd
    sctpState->client_fd = client;

    return sctpState;
}

/**
 * Accepting a new passive connection
 * */
void sctp_passive_accept(struct selector_key *key)
{
    printf("Inside passive accept in SCTP\n");

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

    /*
    // We have to handle the SIGPIPE signal to avoid the server getting killed
    struct sigaction sa;
    sa.sa_handler = sctp_sigpipe_handler;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, 0) == -1)
    {
        // TODO: HANDLE ERROR
    }
    */
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

    sctp_destroy(state);
}

///////////////////////////////////////////////////////////////////
// HANDLERS
///////////////////////////////////////////////////////////////////

/** Gets the struct (sctp *) from the selector key */
#define ATTACHMENT(key) ((struct sctp *)(key)->data)

static void
sctp_read(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);

    // Result, next state to move in the process
    unsigned ret = SCTP_RESPONSE;

    // Setting the current key for the SIGPIPE handler
    current_key = key;

    // If the request returns not the appropiate state, the fd is unregistered
    ret = handle_request(key);
    if (ret != SCTP_RESPONSE)
    {
        selector_unregister_fd(key->s, d->client_fd);
    }
}

static void
sctp_write(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);

    // Result, next state to move in the process
    unsigned ret = SCTP_REQUEST;

    ssize_t n;
    size_t nr;
    uint8_t *ptr = buffer_read_ptr(&d->buffer_write, &nr);

    // TYPE -> TYPE_NOTYPE + CMD -> CMD_NOCMD = LOGIN
    printf("\n\nResponding to SCTP client state %d, cmd %d and type %d\n\n", d->state, d->datagram.data.cmd, d->datagram.data.type);
    if (d->datagram.data.cmd == CMD_NOCMD && d->datagram.data.type == TYPE_NOTYPE)
    {
        if (d->state == SCTP_ERROR)
        {
            printf("Got login error\n");
            uint8_t login_error_data[] = {SCTP_VERSION, 0x01}; // VERSION 1, ERROR 1
            n = sctp_sendmsg(key->fd, (void *)login_error_data, N(login_error_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
        }
        else
        {
            printf("Got login ok\n");
            uint8_t login_success_data[] = {SCTP_VERSION, 0x00}; // VERSION 1, ERROR 0
            n = sctp_sendmsg(key->fd, (void *)login_success_data, N(login_success_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
        }
    }
    else
    {
        // TODO: IMPLEMENT FUTURE STATES
    }

    if (n > 0)
    {
        // Setting the fd to read.
        if (SELECTOR_SUCCESS != selector_set_interest(key->s, key->fd, OP_READ))
        {
            printf("Removing in ok fd\n");
            // Error setting interest, unregister
            selector_unregister_fd(key->s, d->client_fd);
        }
        printf("Success fd\n");
    }
    else
    {
        printf("Removing not ok fd\n");
        // Error sending message, unregister
        selector_unregister_fd(key->s, d->client_fd);
    }
}

static void
sctp_close(struct selector_key *key)
{
    sctp_destroy(ATTACHMENT(key));
}

///////////////////////////////////////////////////////////////////
// REQUESTS
///////////////////////////////////////////////////////////////////

static unsigned handle_request(struct selector_key *key)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);
    // Struct for information about the sender
    struct sctp_sndrcvinfo sender_info;
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
        printf("Error in recv SCTP\n");
        return SCTP_ERROR;
    }

    printf("Im processing the request\n");

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
    return SCTP_ERROR;
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
            selector_unregister_fd(key->s, d->client_fd);
            ret = SCTP_ERROR;
        }
        else
        {
            uint8_t *uid = d->paser.up_request.uid;
            uint8_t *pw = d->paser.up_request.pw;
            uint8_t uid_l = d->paser.up_request.uidLen;
            uint8_t pw_l = d->paser.up_request.pwLen;

            // Validating the login request
            auth_valid = validate_user_admin(uid, pw);

            if (auth_valid)
            {
                // Setting the username in the sctp object
                d->username = malloc(uid_l * sizeof(uint8_t));
                memcpy(d->username, uid, uid_l);
                ret = SCTP_RESPONSE;
            }
            else
            {
                ret = SCTP_ERROR;
            }

            // Setting the type and command as no type and no command
            d->datagram.data.cmd = CMD_NOCMD;
            d->datagram.data.type = TYPE_NOTYPE;
        }
    }

    // Liberating the parser
    free_up_req_parser(&d->paser.up_request);

    return ret;
}