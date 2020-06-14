#include "sctpnio.h"

/*
    Pool for the reusing of instances
*/
/** Max amount of items in pool */
static const unsigned max_pool = 50;
/** Actual pool size */
static unsigned pool_size = 0;
/** Actual pool of objects */
static struct sctp *pool = 0;

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
// HANDLERS
///////////////////////////////////////////////////////////////////

/** Gets the struct (sctp *) from the selector key */
#define ATTACHMENT(key) ((struct sctp *)(key)->data)

static const struct fd_handler sctp_handler = {
    .handle_read = sctp_read,
    .handle_write = sctp_write,
    .handle_close = sctp_close,
    .handle_block = NULL,
};

static void
sctp_read(struct selector_key *key)
{
    // Obtain the sctp struct from the selector key
    struct sctp *d = ATTACHMENT(key);

    // If the sctp struct has a logged user, handle its request
    // Else, handle the login request
    if (d->is_logged)
    {
        handle_normal_request(key);
    }
    else
    {
        handle_login_request(key);
    }

    // Set interest to write because the protocol used is 1 request & 1 response
    // If interest set is not successful, unregister the file descriptor
    selector_status ss = selector_set_interest(key->s, key->fd, OP_WRITE);
    if (ss != SELECTOR_SUCCESS)
    {
        selector_unregister_fd(key->s, d->client_fd);
    }
}

static void
sctp_write(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_write(stm, key);

    if (ERROR == st || DONE == st)
    {
        socksv5_done(key);
    }
}

static void
sctp_close(struct selector_key *key)
{
    sctp_destroy(ATTACHMENT(key));
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
    buffer_init(&(sctpState->buffer_write), BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));
    // Read Buffer for the socket(Initialized)
    buffer_init(&(sctpState->buffer_read), BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));

    // Intialize the client_fd
    sockState->client_fd = client;

    return sctpState;
}

/**
 * Accepting a new passive connection
 * */
void sctp_passive_accept(struct selector_key *key)
{
    printf("Inside passive accept\n");

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

    if (SELECTOR_SUCCESS != selector_register(key->s, client, &socks5_handler,
                                              OP_READ, state))
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
// REQUESTS
///////////////////////////////////////////////////////////////////

void handle_normal_request(struct selector_key *key)
{
    // TODO: HANDLE
}

void handle_login_request(struct selector_key *key)
{
    // TODO: HANDLE
}
