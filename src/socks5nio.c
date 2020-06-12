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

    // Initialize the state machine.
    sockState->stm.current = &client_statbl[0]; // The first state is the HELLO_READ state
    sockState->stm.max_state = ERROR;
    sockState->stm.states = client_statbl;
    stm_init(&(sockState->stm));

    // Write Buffer for the socket(Initialized)
    buffer_init(&(sockState->write_buffer), BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));
    // Read Buffer for the socket(Initialized)
    buffer_init(&(sockState->read_buffer), BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));

    // Intialize the client_fd and the server_fd
    sockState->client_fd = client;
    sockState->origin_fd = -1;

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
    printf("Inside passive accept\n");

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

//------------------------------STATE FUNCTION HANDLERS--------------------------------

////////////////////////////////////////
// HELLO
////////////////////////////////////////

/** inicializa las variables de los estados HELLO_… */
static void
hello_read_init(const unsigned state, struct selector_key *key)
{
    struct hello_st *d = &ATTACHMENT(key)->client.hello;

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
    hello_parser_init(&d->parser);
}

static void
hello_read_close(const unsigned state, struct selector_key *key)
{   
    bool errored;
    struct hello_st *d = &ATTACHMENT(key)->client.hello;
    hello_done_parsing(&d->parser, &errored);
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
        if (hello_is_done(st, &error) && !error){
            if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE))
            {   
                // Process the message 
                ret = hello_process(d);
                // Save the auth method in sockState.
                ATTACHMENT(key)->auth = d->method;
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

/** Process the hello message and check if its valid. */
static unsigned
hello_process(const struct hello_st *d)
{
    unsigned ret = HELLO_WRITE;
    uint8_t *methods = d->parser.auth;
    uint8_t methods_c = d->parser.nauth;
    uint8_t m = SOCKS_HELLO_NO_ACCEPTABLE_METHODS;

    /** TODO: Change to accept multiple methods 
     * For now only accepting NO_AUTH
    */
    for (int i = 0; i < methods_c; i++)
    {

        if (methods[i] == SOCKS_HELLO_NOAUTHENTICATION_REQUIRED)
        {
            m = SOCKS_HELLO_NOAUTHENTICATION_REQUIRED;
        }
    }

    // Save the version and the selected method in the write buffer of hello_st
    if (-1 == hello_marshall(d->wb, m))
    {
        ret = ERROR;
    }

    return ret;
}

////////////////////////////////////////
// HELLO_WRITE
////////////////////////////////////////

/** Writes the version and the selected method 
 * This is done in the init because we already have the info.
*/
static void
hello_write_init(const unsigned state, struct selector_key *key)
{
}

static void
hello_write_close(const unsigned state, struct selector_key *key)
{

    struct socks5 * sock_state = ATTACHMENT(key);

    // Reset read and write buffer for reuse.
    buffer_reset(&sock_state->write_buffer);
    buffer_reset(&sock_state->read_buffer);

    /** TODO: Free memory of hello_st */
    

}

/** lee todos los bytes del mensaje de tipo `hello' y inicia su proceso */
static unsigned
hello_write(struct selector_key *key)
{

    struct hello_st *d = &ATTACHMENT(key)->client.hello;
    ssize_t n;
    unsigned ret = REQUEST_READ;
    size_t nr;
    uint8_t *buffer_read = buffer_read_ptr(d->wb, &nr);

    // Get the data from the write buffer
    uint8_t data[] = {buffer_read[0], buffer_read[1]};
    buffer_read_adv(d->wb, 2);

    // Send the version and the method.
    n = send(key->fd, data, 2, 0);
    if (n > 0){
        // Setting the fd to read.
        if (SELECTOR_SUCCESS != selector_set_interest_key(key, OP_READ)){
            ret = ERROR;
        }
    }
    else{
        ret = ERROR;
    }

    // Check if there is an acceptable method, if not --> Error
    if(data[1] == SOCKS_HELLO_NO_ACCEPTABLE_METHODS){
        ret = ERROR;
    }
            

    return ret;
}

////////////////////////////////////////
// USERPASS_READ
////////////////////////////////////////

static void
userpass_read_init(const unsigned state, struct selector_key *key){

}

static void
userpass_read_close(const unsigned state, struct selector_key *key)
{   
 
}

static unsigned
userpass_read(struct selector_key *key)
{

}

////////////////////////////////////////
// USERPASS_WRITE
////////////////////////////////////////

static void
userpass_write_init(const unsigned state, struct selector_key *key){

}

static void
userpass_write_close(const unsigned state, struct selector_key *key)
{   
 
}

static unsigned
userpass_write(struct selector_key *key)
{

}

////////////////////////////////////////
// REQUEST
////////////////////////////////////////

/** Frees the parser used */
static void
request_close(const unsigned state, struct selector_key *key)
{
    // Sock5 state
    struct socks5 * s = ATTACHMENT(key);

    // Reset read and write buffer for reuse.
    buffer_reset(&s->write_buffer);
    buffer_reset(&s->read_buffer);

    /** TODO: Free everything */

    // All temporal for testing... -----> DELTE SHORTLY

    if(s->origin_info.ip_selec == IPv4){
        printf("    IPv4: ");
        for(int i = 0; i < IP_V4_ADDR_SIZE ; i++) {
            printf("%d ", s->origin_info.ipv4_addrs[0][i]);
        }
        printf("\n");
    }
    else if(s->origin_info.ip_selec == IPv6){
        printf("    IPv6: ");
        for(int i = 0; i < IP_V6_ADDR_SIZE ; i++) {
            printf("%d ", s->origin_info.ipv6_addrs[0][i]);
        }
        printf("\n");
    }
    else{
        printf("    dom: ");
        for(int i = 0; i < s->origin_info.resolve_addr_len ; i++) {
            printf("%d ", s->origin_info.resolve_addr[i]);
        }
        printf("\n");
    }
    printf("    port: %d%d\n", s->origin_info.port[0], s->origin_info.port[1]);

    
    printf("Im forever stuck in request_close...\n");
}

static void
request_init(const unsigned state, struct selector_key *key)
{
    struct request_st *d = &ATTACHMENT(key)->client.request;

    // Adding the read buffer
    d->rb = &(ATTACHMENT(key)->read_buffer);

    // Parser init
    connection_req_parser_init(d->parser);

}

static unsigned
request_process(struct selector_key *key, struct request_st *d);

static unsigned
request_read(struct selector_key *key)
{
    // Getting the state struct
    struct request_st *d = &ATTACHMENT(key)->client.request;
    // Getting the read buffer
    buffer *b = d->rb;
    unsigned ret = REQUEST_READ;
    bool error = false;
    uint8_t *ptr;
    size_t count;   //Maximum data that can set in the buffer
    ssize_t n;

    // Setting the buffer to read
    ptr = buffer_write_ptr(b, &count);
    // Receiving data
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {   
        // Notifying the data to the buffer
        buffer_write_adv(b, n);
        // Consuming the message
        const enum connection_req_state st = connection_req_consume_message(b, d->parser, &error);
        //If done parsing and no error
        if (connection_req_done_parsing(d->parser, &error) && !error){
           ret = request_process(key, d);
        }

        if(error){
            /** TODO: HANDLE ERRORS */
            switch (st)
            {
            case CONN_REQ_ERR_INV_VERSION:
                break;
            case CONN_REQ_ERR_INV_CMD:
                break;
            case CONN_REQ_ERR_INV_RSV:
                break;
            case CONN_REQ_ERR_INV_DSTADDR:
                break;
            case CONN_REQ_GENERIC_ERR:
            default:
                break;
            }
        }
        
    }
    else
    {
        ret = ERROR;
    }

    // Setting the fd for WRITE --> RESOLVE and REQUEST both will need to write
    if (SELECTOR_SUCCESS != selector_set_interest_key(key, OP_WRITE)){
            ret = ERROR;
    }


    return error ? ERROR : ret;
}

/** 
 * Processing of the request 
 * 
*/
static unsigned
request_process(struct selector_key *key, struct request_st *d)
{
    unsigned ret = ERROR; 
    struct socks5 *s = ATTACHMENT(key);
    connection_req_parser r_parser = d->parser;
    // Request information obtained by the parser
    uint8_t cmd = r_parser->finalMessage.cmd;
    uint8_t addr_t = r_parser->socks_5_addr_parser->type;
    uint8_t addr_l = r_parser->socks_5_addr_parser->addrLen;
    uint8_t * addr = r_parser->socks_5_addr_parser->addr;
    uint8_t * dst_port = r_parser->finalMessage.dstPort;

    // We just support connect cmd
    if(cmd == TCP_IP_STREAM){

        // The original quantities of ips stored its 0.
        s->origin_info.ipv4_c = s->origin_info.ipv6_c = 0;

        // Determine the next state depending on the type of address given
        switch (addr_t)
        {
        case IPv4:
            // Save the address
            memcpy(s->origin_info.ipv4_addrs[s->origin_info.ipv4_c++], addr, IP_V4_ADDR_SIZE);
            s->origin_info.ip_selec =IPv4;
            //Save the port.
            memcpy(s->origin_info.port, dst_port, 2);
            // We have all the info to connect
            ret = CONNECTING;
            break;

        case IPv6:
            // Save the address
            memcpy(s->origin_info.ipv6_addrs[s->origin_info.ipv6_c++], addr, IP_V6_ADDR_SIZE);
            s->origin_info.ip_selec =IPv6;
            //Save the port.
            memcpy(s->origin_info.port, dst_port, 2);
            // We have all the info to connect
            ret = CONNECTING;
            break;

        case DOMAIN_NAME:
            // Save the domain name
            s->origin_info.resolve_addr = malloc(addr_l);
            memcpy(s->origin_info.resolve_addr, addr, addr_l); 
            s->origin_info.resolve_addr_len= addr_l;

            //Save the port.
            memcpy(s->origin_info.port, dst_port, 2);

            // We need to resolve the domain name.
            ret = RESOLVE;
            break;

        default:
            ret = ERROR;
            break;
        }
    }
    else{
        ret = ERROR;
    }

    return ret;
}

////////////////////////////////////////
// RESOLVE
////////////////////////////////////////

////////////////////////////////////////
// CONNECTING
////////////////////////////////////////

void connecting_init(const unsigned state, struct selector_key * key) {

    printf("Connecting Init\n");
    struct connecting_st * d = &ATTACHMENT(key)->orig.conn;

    d->rb = &(ATTACHMENT(key)->read_buffer);

    struct sockaddr_storage orig_addr;
    socklen_t orig_addr_len = sizeof(orig_addr);


}

static unsigned connecting_read(struct selector_key * key) {

}

void connecting_close(const unsigned state, struct selector_key * key) {

}
////////////////////////////////////////
// COPY
////////////////////////////////////////



//------------------------STATES DEFINITION--------------------------------------

/** definición de handlers para cada estado */
static const struct state_definition client_statbl[] = {
    {
        .state = HELLO_READ,
        .on_arrival = hello_read_init,
        .on_departure = hello_read_close,
        .on_read_ready = hello_read,
    },
    {   .state = HELLO_WRITE,
        .on_arrival = hello_write_init,
        .on_departure = hello_write_close,
        .on_write_ready = hello_write
    },
    {  .state = USERPASS_READ,
        .on_arrival = userpass_read_init,
        .on_departure = userpass_read_close,
        .on_write_ready = userpass_read,
    },
    {  .state = USERPASS_WRITE,
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
    },
    {
        .state = CONNECTING,
        .on_arrival = connecting_init,
        .on_read_ready = connecting_read,
        .on_departure = connecting_close,
    },
    {
        .state = REPLY,
    },
    {
        .state = COPY,
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
