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

// TYPE + CMD handlers
static unsigned handle_list_users(struct selector_key *key);
static unsigned handle_user_create(struct selector_key *key, buffer *b);

// Response preparers
static uint8_t *prepare_list_users(uint8_t **users, int count);

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
        printf("I was unregistered\n");
        selector_unregister_fd(key->s, d->client_fd);
    }

    printf("I REALLY FINISHED\n");
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
    printf("\n\nResponding to SCTP client state %d, cmd %d and type %d\n\n", d->state, d->info.cmd, d->info.type);
    switch (d->info.type)
    {
    case TYPE_USERS:
        switch (d->info.cmd)
        {
        case CMD_LIST:
            if (d->state == SCTP_ERROR)
            {
                uint8_t user_list_error_data[] = {d->info.type, d->info.cmd, 0x01, 0, 0}; // VERSION 1, ERROR 1
                n = sctp_sendmsg(key->fd, (void *)user_list_error_data, N(user_list_error_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            }
            else
            {
                // Getting the representation for the users
                uint8_t *users = prepare_list_users(d->datagram.user_list.users, d->datagram.user_list.user_count);
                // Adding the info to the buffer
                uint8_t *user_list_data = calloc(4 + strlen(users), sizeof(uint8_t));
                user_list_data[0] = d->info.type;
                user_list_data[1] = d->info.cmd;
                user_list_data[2] = 0x00;
                user_list_data[3] = d->datagram.user_list.user_count;
                memcpy(&user_list_data[4], users, strlen(users));
                n = sctp_sendmsg(key->fd, (void *)user_list_data, 4 + strlen(users), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            }
            break;
        case CMD_CREATE:
            if (d->state == SCTP_ERROR)
            {
                uint8_t user_create_error_data[] = {d->info.type, d->info.cmd, 0x01, SCTP_VERSION}; // VERSION 1, ERROR 1
                n = sctp_sendmsg(key->fd, (void *)user_create_error_data, N(user_create_error_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            }
            else
            {
                uint8_t user_create_success_data[] = {d->info.type, d->info.cmd, 0x00, SCTP_VERSION}; // VERSION 1, ERROR 0
                n = sctp_sendmsg(key->fd, (void *)user_create_success_data, N(user_create_success_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
            }
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
            break;
        case CMD_EDIT:
            break;
        // CMD not allowed
        default:
            ret = SCTP_ERROR;
            break;
        }
        break;
    case TYPE_METRICS:
        switch (d->info.cmd)
        {
        case CMD_LIST:
            break;
            // CMD not allowed
        default:
            ret = SCTP_ERROR;
            break;
        }
        break;
    // TYPE not allowed
    case TYPE_NOTYPE:
        switch (d->info.cmd)
        {
        case CMD_NOCMD:
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
            break;
        default:
            ret = SCTP_ERROR;
            break;
        }
        break;
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

static TYPE determine_type(int val)
{
    switch (val)
    {
    case TYPE_NOTYPE:
        return TYPE_NOTYPE;
    case TYPE_CONFIG:
        return TYPE_CONFIG;
    case TYPE_METRICS:
        return TYPE_METRICS;
    case TYPE_USERS:
        return TYPE_USERS;
    default:
        return TYPE_NOTYPE;
    }
}

static CMD determine_cmd(int val)
{
    switch (val)
    {
    case CMD_CREATE:
        return CMD_CREATE;
    case CMD_EDIT:
        return CMD_EDIT;
    case CMD_LIST:
        return CMD_LIST;
    case CMD_NOCMD:
        return CMD_NOCMD;
    default:
        return CMD_NOCMD;
    }
}

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

    printf("This user logged %d\n", d->is_logged);

    if (d->is_logged)
    {
        printf("Normal\n");
        ret = handle_normal_request(key);
    }
    else
    {
        printf("Login\n");
        ret = handle_login_request(key);
    }

    printf("Handler read finished\n");

    return ret;
}

static unsigned handle_normal_request(struct selector_key *key)
{
    // Getting the sctp struct
    struct sctp *d = ATTACHMENT(key);
    // Getting the buffer to read the data
    buffer *b = &d->buffer_read;
    // Boolean for parser error and auth validity
    bool error = false, auth_valid = false;
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
        printf("Buffer cannot read");
        return SCTP_ERROR;
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
        printf("Buffer cannot read");
        return SCTP_ERROR;
    }

    // Advancing the buffer
    buffer_read_adv(b, 1);

    TYPE type = determine_type(given_type);
    CMD cmd = determine_cmd(given_cmd);

    d->info.cmd = cmd;
    d->info.type = type;

    printf("Setting CMD and TYPE \n");

    printf("The tyype is %d and cmd is %d\n", d->info.type, d->info.cmd);
    // Switching TYPE and CMD to determine available operations
    switch (type)
    {
    case TYPE_USERS:
        switch (cmd)
        {
        case CMD_LIST:
            printf("Handling user list\n");
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
        switch (cmd)
        {
        case CMD_LIST:
            break;
        case CMD_EDIT:
            break;
        // CMD not allowed
        default:
            ret = SCTP_ERROR;
            break;
        }
        break;
    case TYPE_METRICS:
        switch (cmd)
        {
        case CMD_LIST:
            break;
            // CMD not allowed
        default:
            ret = SCTP_ERROR;
            break;
        }
        break;
    // TYPE not allowed
    case TYPE_NOTYPE:
        ret = SCTP_ERROR;
        break;
    }

    // Compacting the buffer
    buffer_compact(b);

    if (SELECTOR_SUCCESS != selector_set_interest(key->s, key->fd, OP_WRITE))
    {
        ret = SCTP_ERROR;
    }

    printf("Finished stting CMD and TYPE\n");

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
                d->is_logged = true;
                ret = SCTP_RESPONSE;
            }
            else
            {
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
    printf("Handling user list\n");
    // Getting all the users
    uint8_t count = 0;
    uint8_t *users[MAX_USERS];
    list_user_admin(users, &count);
    if (users == NULL)
    {
        printf("NULL USER POINTER \n");
        return SCTP_ERROR;
    }

    // Adding the list and the count to the requets data
    ATTACHMENT(key)->datagram.user_list.users = users;
    ATTACHMENT(key)->datagram.user_list.user_count = count;

    return SCTP_RESPONSE;
}

static unsigned handle_user_create(struct selector_key *key, buffer *b)
{
    printf("Creating user\n\n");
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
            printf("Could not set interest\n\n");
            ret = SCTP_ERROR;
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

            printf("Result of user create %d \n", error);

            ret = !error ? SCTP_RESPONSE : SCTP_ERROR;
        }
    }

    // Liberating the parser
    free_up_req_parser(&d->paser.up_request);

    return ret;
}

///////////////////////////////////////////////////////////////////
// RESPONSE PREPARERS
///////////////////////////////////////////////////////////////////

static uint8_t *prepare_list_users(uint8_t **users, int count)
{
    int i = 0;
    int previousLength = 0;
    uint8_t *value = NULL;
    for (i = 0; i < count; i++)
    {
        if (i == 0)
        {
            value = realloc(value, strlen(users[i]));
            if (value == NULL)
            {
                return NULL;
            }
            sprintf(value, "%s", users[i]);
        }
        else
        {
            value = realloc(value, strlen(users[i]) + 1 + strlen(value));
            if (value == NULL)
            {
                return NULL;
            }
            sprintf(value, "%s,%s", value, users[i]);
        }
    }
    return value;
}