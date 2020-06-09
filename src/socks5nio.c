#include "socks5nio.h"
/**
 * socks5nio.c  - controla el flujo de un proxy SOCKSv5 (sockets no bloqueantes)
 */

// Tabla con informacion de todos los estados.
static const struct state_definition client_statbl[];

/*
    Pool for the reusing of instances
*/
/** Max amount of items in pool */
static const unsigned max_pool = 50;
/** Actual pool size */
static unsigned pool_size = 0;
/** Actual pool of objects */
static struct socks5 *pool = 0;

//-----------------------FUNCIONES DE MANEJO DE SOCKSv5----------------------------

/** 
 * Creacion de un nuevo socks5.
 */
static struct socks5 *socks5_new(const int client)
{

    // Initialize the Socks5 structure which contain the state machine and other info for the socket.
    struct socks5 *sockState = malloc(sizeof(struct socks5));
    if (sockState == NULL)
    {
        perror("Error: Initizalizing null Socks5\n");
    }

    ////////////////////////////////////////////////////////////////////
    // STATE VARIABLES
    ////////////////////////////////////////////////////////////////////

    sockState->stm.current = HELLO_READ;
    sockState->stm.initial = HELLO_READ;
    sockState->stm.max_state = MAX_STATES;
    sockState->stm.states = client_statbl;
    stm_init(&(sockState->stm));

    // Write Buffer for the socket(Initialized)
    //buffer *writeBuffer = malloc(sizeof(buffer));
    //buffer_init(writeBuffer, BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));
    //sockState->writeBuffer = writeBuffer
}

/** realmente destruye */
static void
socks5_destroy_(struct socks5 *s)
{
    if (s->origin_resolution != NULL)
    {
        freeaddrinfo(s->origin_resolution);
        s->origin_resolution = 0;
    }
    free(s);
}

/**
 * destruye un  `struct socks5', tiene en cuenta las referencias
 * y el pool de objetos.
 */
static void
socks5_destroy(struct socks5 *s)
{
    if (s == NULL)
    {
        // nada para hacer
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
                socks5_destroy_(s);
            }
        }
    }
    else
    {
        s->references -= 1;
    }
}

void socksv5_pool_destroy(void)
{
    struct socks5 *next, *s;
    for (s = pool; s != NULL; s = next)
    {
        next = s->next;
        free(s);
    }
}

//--------------------------------HANDLERS----------------------------------

/** obtiene el struct (socks5 *) desde la llave de selección  */
#define ATTACHMENT(key) ((struct socks5 *)(key)->data)

/* declaración forward de los handlers de selección de una conexión
 * establecida entre un cliente y el proxy.
 */
static void socksv5_read(struct selector_key *key);
static void socksv5_write(struct selector_key *key);
static void socksv5_block(struct selector_key *key);
static void socksv5_close(struct selector_key *key);
static const struct fd_handler socks5_handler = {
    .handle_read = socksv5_read,
    .handle_write = socksv5_write,
    .handle_close = socksv5_close,
    .handle_block = socksv5_block,
};

/** Intenta aceptar la nueva conexión entrante*/
void socksv5_passive_accept(struct selector_key *key)
{
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct socks5 *state = NULL;

    const int client = accept(key->fd, (struct sockaddr *)&client_addr,
                              &client_addr_len);
    if (client == -1)
    {
        goto fail;
    }
    if (selector_fd_set_nio(client) == -1)
    {
        goto fail;
    }
    state = socks5_new(client);
    if (state == NULL)
    {
        // sin un estado, nos es imposible manejaro.
        // tal vez deberiamos apagar accept() hasta que detectemos
        // que se liberó alguna conexión.
        goto fail;
    }
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
    socks5_destroy(state);
}

////////////////////////////////////////////////////////////////////////////////
// STATE FUNCTION HANDLERS
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////
// HELLO
////////////////////////////////////////

/** callback del parser utilizado en `read_hello' */
static void
on_hello_method(struct hello_parser *p, const uint8_t method)
{
    uint8_t *selected = p->data;

    if (SOCKS_HELLO_NOAUTHENTICATION_REQUIRED == method)
    {
        *selected = method;
    }
}

/** inicializa las variables de los estados HELLO_… */
static void
hello_read_init(const unsigned state, struct selector_key *key)
{
    struct hello_st *d = &ATTACHMENT(key)->client.hello;

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
    d.parser.data = &d->method;
    d->parser.on_authentication_method = on_hello_method, hello_parser_init(&d->parser);
}

static void
hello_read_close(const unsigned state, struct selector_key *key)
{
    struct hello_st *d = &ATTACHMENT(key)->client.hello;
    hello_done_parsing(&d->parser);
}

static unsigned
hello_process(const struct hello_st *d);

/** lee todos los bytes del mensaje de tipo `hello' y inicia su proceso */
static unsigned
hello_read(struct selector_key *key)
{
    struct hello_st *d = &ATTACHMENT(key)->client.hello;
    unsigned ret = HELLO_READ;
    bool error = false;
    uint8_t *ptr;
    size_t count;
    ssize_t n;

    ptr = buffer_write_ptr(d->rb, &count);
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {
        buffer_write_adv(d->rb, n);
        const enum hello_state st = hello_consume(d->rb, &d->parser, &error);
        if (hello_is_done(st, 0))
        {
            if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE))
            {
                ret = hello_process(d);
            }
            else
            {
                ret = ERROR;
            }
        }
    }
    else
    {
        ret = ERROR;
    }

    return error ? ERROR : ret;
}

/** procesamiento del mensaje `hello' */
static unsigned
hello_process(const struct hello_st *d)
{
    unsigned ret = HELLO_WRITE;

    uint8_t m = d->method;
    const uint8_t r = (m == SOCKS_HELLO_NO_ACCEPTABLE_METHODS) ? 0xFF : 0x00;
    if (-1 == hello_marshall(d->wb, r))
    {
        ret = ERROR;
    }
    if (SOCKS_HELLO_NO_ACCEPTABLE_METHODS == m)
    {
        ret = ERROR;
    }
    return ret;
}

////////////////////////////////////////
// HELLO_WRITE
////////////////////////////////////////

/** inicializa las variables de los estados HELLO_… */
static void
hello_write_init(const unsigned state, struct selector_key *key)
{
    /*
    struct hello_st *d = &ATTACHMENT(key)->client.hello;

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
    d->parser.data = &d->method;
    d->parser.on_authentication_method = on_hello_method, hello_parser_init(&d->parser);
    */
}

static void
hello_write_close(const unsigned state, struct selector_key *key)
{
    /*
    struct hello_st *d = &ATTACHMENT(key)->client.hello;
    hello_done_parsing(&d->parser);
    */
}

/** lee todos los bytes del mensaje de tipo `hello' y inicia su proceso */
static unsigned
hello_write(struct selector_key *key)
{
    /*
    struct hello_st *d = &ATTACHMENT(key)->client.hello;
    unsigned ret = HELLO_READ;
    bool error = false;
    uint8_t *ptr;
    size_t count;
    ssize_t n;

    ptr = buffer_write_ptr(d->rb, &count);
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {
        buffer_write_adv(d->rb, n);
        const enum hello_state st = hello_consume(d->rb, &d->parser, &error);
        if (hello_is_done(st, 0))
        {
            if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE))
            {
                ret = hello_process(d);
            }
            else
            {
                ret = ERROR;
            }
        }
    }
    else
    {
        ret = ERROR;
    }

    return error ? ERROR : ret;
    */
}

////////////////////////////////////////
// REQUEST_READ
////////////////////////////////////////

////////////////////////////////////////
// RESOLVE
////////////////////////////////////////

////////////////////////////////////////
// CONNECTING
////////////////////////////////////////

////////////////////////////////////////
// COPY
////////////////////////////////////////

////////////////////////////////////////
// DONE
////////////////////////////////////////

////////////////////////////////////////
// ERRORS
////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// STATES DEFINITION
////////////////////////////////////////////////////////////////////////////////

/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state = HELLO_READ,
        .on_arrival = hello_read_init,
        .on_departure = hello_read_close,
        .on_read_ready = hello_read,
    },
    {
        .state = HELLO_WRITE,
        .on_arrival = hello_write_init,
        .on_departure = hello_write_close,
        .on_write_ready = hello_write},
    {
        .state = REQUEST_READ,
    },
    {
        .state = RESOLVE,
    },
    {
        .state = CONNECTING,
    },
    {
        .state = REPLY,
    },
    {
        .state = COPY,
    },
    {
        .state = DONE,
    },
    {
        .state = ERROR,
    }};

////////////////////////////////////////////////////////////////////////////////
// SOCKS5 HANDLERS
////////////////////////////////////////////////////////////////////////////////
// Handlers top level de la conexión pasiva.
// son los que emiten los eventos a la maquina de estados.
static void socksv5_done(struct selector_key *key);

static void
socksv5_read(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_read(stm, key);

    if (ERROR == st || DONE == st)
    {
        socksv5_done(key);
    }
}

static void
socksv5_write(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_write(stm, key);

    if (ERROR == st || DONE == st)
    {
        socksv5_done(key);
    }
}

static void
socksv5_block(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_block(stm, key);

    if (ERROR == st || DONE == st)
    {
        socksv5_done(key);
    }
}

static void
socksv5_close(struct selector_key *key)
{
    socks5_destroy(ATTACHMENT(key));
}

static void
socksv5_done(struct selector_key *key)
{
    const int fds[] = {
        ATTACHMENT(key)->client_fd,
        ATTACHMENT(key)->origin_fd,
    };
    for (unsigned i = 0; i < N(fds); i++)
    {
        if (fds[i] != -1)
        {
            if (SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i]))
            {
                abort();
            }
            close(fds[i]);
        }
    }
}
