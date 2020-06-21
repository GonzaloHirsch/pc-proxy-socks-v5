#include "socks5nio/socks5nio.h"

#include "socks5nio/socks5nio_hello.h"
#include "socks5nio/socks5nio_userpass.h"
#include "socks5nio/socks5nio_request.h"
#include "socks5nio/socks5nio_resolve.h"
#include "socks5nio/socks5nio_connecting.h"
#include "socks5nio/socks5nio_copy.h"

/**
 * socks5nio.c  - controla el flujo de un proxy SOCKSv5 (sockets no bloqueantes)
 */

// Tabla con informacion de todos los estados.
static const struct state_definition client_statbl[];

/*
    Pool for the reusing of instances
*/
/** Max amount of items in pool */
//static const unsigned max_pool = 50;
/** Actual pool size */
//static unsigned pool_size = 0;
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

    // Initialize the state machine.
    sockState->stm.current = &client_statbl[0]; // The first state is the HELLO_READ state
    sockState->stm.max_state = ERROR;
    sockState->stm.states = client_statbl;
    sockState->stm.initial = HELLO_READ;
    stm_init(&(sockState->stm));

    // Write Buffer for the socket(Initialized)
    buffer_init(&(sockState->write_buffer), BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));
    // Read Buffer for the socket(Initialized)
    buffer_init(&(sockState->read_buffer), BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));

    // Intialize the client_fd and the server_fd
    sockState->client_fd = client;
    sockState->origin_fd = sockState->origin_fd6 = sockState->sel_origin_fd = -1;

    sockState->reply_type = -1;
    sockState->references = 1;
    sockState->origin_resolution = NULL;

    return sockState;
}

/** realmente destruye */
static void
socks5_destroy_(struct socks5 *s)
{
    if (s->origin_resolution != NULL)
    {
        //freeaddrinfo(s->origin_resolution);
        s->origin_resolution = 0;
    }
    if (s != NULL)
    {
        // Liberating the username if allocated
        if (s->username != NULL)
        {
            free(s->username);
        }
        // Freeing the address name if allocated
        if (s->origin_info.resolve_addr != NULL)
        {
            free(s->origin_info.resolve_addr);
        }
        // Liberating the buffers
        free(s->read_buffer.data);
        free(s->write_buffer.data);
        // Liberating the struct
        free(s);
    }
}

/**
 * destruye un  `struct socks5', tiene en cuenta las referencias
 * y el pool de objetos.
 */
/*
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
*/

void socksv5_pool_destroy(void)
{
    struct socks5 *next, *s;
    for (s = pool; s != NULL; s = next)
    {
        next = s->next;
        free(s);
    }
}

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
    // If everything is ok, add the metrics for the current/historic connections
    add_current_connections(1);
    add_historic_connections(1);
    return;
fail:
    if (client != -1)
    {
        close(client);
    }
    socks5_destroy_(state);
}

//------------------------------STATE FUNCTION HANDLERS--------------------------------

//------------------------STATES DEFINITION--------------------------------------

/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state = HELLO_READ,
        .on_arrival = hello_read_init,
        .on_departure = hello_read_close,
        .on_read_ready = hello_read,
    },
    {.state = HELLO_WRITE,
     .on_arrival = hello_write_init,
     .on_departure = hello_write_close,
     .on_write_ready = hello_write},
    {
        .state = USERPASS_READ,
        .on_arrival = userpass_read_init,
        .on_departure = userpass_read_close,
        .on_read_ready = userpass_read,
    },
    {
        .state = USERPASS_WRITE,
        .on_arrival = userpass_write_init,
        .on_departure = userpass_write_close,
        .on_write_ready = userpass_write,
    },
    {
        .state = REQUEST_READ,
        .on_arrival = request_init,
        .on_departure = request_close,
        .on_read_ready = request_read,
    },
    {
        .state = RESOLVE,
        .on_arrival = resolve_init,
        .on_departure = resolve_close,
        .on_read_ready = resolve_read,
        .on_write_ready = resolve_write,

    },
    {
        .state = CONNECTING,
        .on_arrival = connecting_init,
        .on_read_ready = connecting_read,
        .on_write_ready = connecting_write,
        .on_departure = connecting_close,
    },
    {   .state = COPY,
        .on_arrival = copy_init,
        .on_read_ready = copy_read,
        .on_write_ready = copy_write,
        .on_departure = copy_close
        },
    {
        .state = DONE,
        // For now, no need to define any handlers, all in sockv5_done
    },
    {
        .state = ERROR,
        // No now, no need to define any handlers, all in sockv5_done
    }};

////////////////////////////////////////////////////////////////////////////////
// SOCKS5 HANDLERS
////////////////////////////////////////////////////////////////////////////////

// Handlers top level de la conexión pasiva.
// son los que emiten los eventos a la maquina de estados.
void socksv5_done(struct selector_key *key);

void socksv5_read(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_read(stm, key);

    if (ERROR == st || DONE == st)
    {
        socksv5_done(key);
    }
}

void socksv5_write(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_write(stm, key);

    if (ERROR == st || DONE == st)
    {
        socksv5_done(key);
    }
}

void socksv5_block(struct selector_key *key)
{
    struct state_machine *stm = &ATTACHMENT(key)->stm;
    const enum socks_v5state st = stm_handler_block(stm, key);

    if (ERROR == st || DONE == st)
    {
        socksv5_done(key);
    }
}

void socksv5_close(struct selector_key *key)
{
    // Removing the current connection from the metrics
    remove_current_connections(1);

    close(key->fd);
}

void socksv5_done(struct selector_key *key)
{
    const int fds[] = {
        ATTACHMENT(key)->client_fd,
        ATTACHMENT(key)->origin_fd,
        ATTACHMENT(key)->origin_fd6,
    };
    for (unsigned i = 0; i < N(fds); i++)
    {
        if (fds[i] != -1)
        {
            if (SELECTOR_SUCCESS != selector_unregister_fd(key->s, fds[i]))
            {
                printf("Socks is done for %d\n", fds[i]);
                abort();
            }
            //close(fds[i]);
        }
    }

    // Calling the destroy_ method because the object pool is not implemented
    socks5_destroy_(ATTACHMENT(key));
}
