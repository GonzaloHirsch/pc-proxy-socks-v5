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

static void socks5_origin_read(struct selector_key *key);
static void socks5_origin_write(struct selector_key *key);
static void socks5_origin_close(struct selector_key *key);
static void socks5_origin_block(struct selector_key *key);
static const struct fd_handler socks5_origin_handler = {
    .handle_read = socks5_origin_read,
    .handle_write = socks5_origin_write,
    .handle_close = socks5_origin_close,
    .handle_block = socks5_origin_block,
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
   struct socks5 *sock_state = ATTACHMENT(key);

   free_hello_parser(&sock_state->client.hello.parser);
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
        if (hello_is_done(st, &error) && !error)
        {
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


    for (int i = 0; i < methods_c; i++)
    {    
        if (methods[i] == SOCKS_HELLO_NOAUTHENTICATION_REQUIRED){
            m = SOCKS_HELLO_NOAUTHENTICATION_REQUIRED;
            break;  // Priority to no auth required
        }
        else if (methods[i] == SOCKS_HELLO_USERPASS){
            m = SOCKS_HELLO_USERPASS;
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


static void
hello_write_init(const unsigned state, struct selector_key *key)
{
}

static void
hello_write_close(const unsigned state, struct selector_key *key)
{

    struct socks5 *sock_state = ATTACHMENT(key);

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
    unsigned ret = ERROR;
    size_t nr;
    uint8_t *buffer_read = buffer_read_ptr(d->wb, &nr);
    uint8_t auth = ATTACHMENT(key)->auth;

    // Get the data from the write buffer
    uint8_t data[] = {buffer_read[0], buffer_read[1]};
    buffer_read_adv(d->wb, 2);

    // Send the version and the method.
    n = send(key->fd, data, 2, 0);
    if (n > 0)
    {
        // Setting the fd to read.
        if (SELECTOR_SUCCESS != selector_set_interest_key(key, OP_READ))
        {
            ret = ERROR;
        }
    }
    else
    {
        ret = ERROR;
    }

    // Check if there is an acceptable method, if not --> Error
    if(auth == SOCKS_HELLO_NOAUTHENTICATION_REQUIRED){
        ret = REQUEST_READ;
    }
    else if(auth == SOCKS_HELLO_USERPASS){
         ret = USERPASS_READ;
    }
   else{
       ret = ERROR;
   }

    return ret;
}

////////////////////////////////////////
// USERPASS_READ
////////////////////////////////////////

static void
userpass_read_init(const unsigned state, struct selector_key *key)
{
    struct socks5 *s = ATTACHMENT(key);

    struct userpass_st * up_s= &s->client.userpass;

    up_s->rb = &(s->read_buffer);
    up_s->wb = &(s->write_buffer);
    up_req_parser_init(&up_s->parser);
}

static void
userpass_read_close(const unsigned state, struct selector_key *key)
{
    struct socks5 *sock_state = ATTACHMENT(key);

    free_up_req_parser(&sock_state->client.userpass.parser);
}

static unsigned
userpass_process(struct userpass_st *up_s, bool * auth_valid);

static unsigned
userpass_read(struct selector_key *key)
{
    struct userpass_st *up_s = &ATTACHMENT(key)->client.userpass;
    unsigned ret = USERPASS_READ;
    bool error = false, auth_valid = false;
    uint8_t *ptr;
    size_t count;
    ssize_t n;

    ptr = buffer_write_ptr(up_s->rb, &count);
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {
        buffer_write_adv(up_s->rb, n);
        // Parse the inofmration
        const enum up_req_state st = up_consume_message(up_s->rb, &up_s->parser, &error);
        if (up_done_parsing(st, &error) && !error)
        {   
            // Set the fd to write for the userpass write state
            if (SELECTOR_SUCCESS == selector_set_interest_key(key, OP_WRITE))
            {
                // Process the user password info.
                ret = userpass_process(up_s, &auth_valid);
                // Save the credential if auth is valid
                if(auth_valid)
                    ATTACHMENT(key)->username = up_s->user;
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

static unsigned
userpass_process(struct userpass_st *up_s, bool * auth_valid){
    unsigned ret = USERPASS_WRITE;
    uint8_t * uid = up_s->parser.uid, * uid_stored;
    uint8_t * pw = up_s->parser.pw, * pw_stored;
    uint8_t uid_l = up_s->parser.uidLen;
    uint8_t pw_l = up_s->parser.pwLen;

    *auth_valid = validate_user_proxy(uid, pw);
    
    if(*auth_valid){            
        up_s->user = malloc(uid_l);
        memcpy(up_s, uid, uid_l);
    }           
            

    // Serialize the auth result in the write buffer for the response.
    if(-1 == up_marshall(up_s->wb, *auth_valid ? AUTH_SUCCESS : AUTH_FAILURE)){
        ret = ERROR;
    }

    return ret;   
    
}

////////////////////////////////////////
// USERPASS_WRITE
////////////////////////////////////////

static void
userpass_write_init(const unsigned state, struct selector_key *key)
{
}

static void
userpass_write_close(const unsigned state, struct selector_key *key)
{
    struct socks5 *sock_state = ATTACHMENT(key);

    // Reset read and write buffer for reuse.
    buffer_reset(&sock_state->write_buffer);
    buffer_reset(&sock_state->read_buffer);

    /** TODO: Free memory of userpass_st */   
}

static unsigned
userpass_write(struct selector_key *key)
{
    struct userpass_st *up_s = &ATTACHMENT(key)->client.userpass;
    ssize_t n;
    unsigned ret = REQUEST_READ;
    size_t nr;
    uint8_t *up_response = buffer_read_ptr(up_s->wb, &nr);

    uint8_t data[] = {up_response[0], up_response[1]};
    buffer_read_adv(up_s->wb, 2);

    // Send the version and the authentication result
    n = send(key->fd, data, 2, 0);
    if (n > 0)
    {
        // Setting the fd to read.
        if (SELECTOR_SUCCESS != selector_set_interest_key(key, OP_READ))
        {
            ret = ERROR;
        }
    }
    else
    {
        ret = ERROR;
    }

    // Check if the authentication was successful if not --> Error
    if (up_response[1] != AUTH_SUCCESS)
    {
        ret = ERROR;
    }

    return ret;
}

////////////////////////////////////////
// REQUEST
////////////////////////////////////////

/** Frees the parser used */
static void
request_close(const unsigned state, struct selector_key *key)
{
    // Sock5 state
    struct socks5 *s = ATTACHMENT(key);

    // Reset read and write buffer for reuse.
    buffer_reset(&s->write_buffer);
    buffer_reset(&s->read_buffer);

    /** TODO: Free everything */

    // All temporal for testing... -----> DELTE SHORTLY

    if (s->origin_info.ip_selec == IPv4)
    {
        printf("    IPv4: ");
        for (int i = 0; i < IP_V4_ADDR_SIZE; i++)
        {
            printf("%d ", s->origin_info.ipv4_addrs[0][i]);
        }
        printf("\n");
    }
    else if (s->origin_info.ip_selec == IPv6)
    {
        printf("    IPv6: ");
        for (int i = 0; i < IP_V6_ADDR_SIZE; i++)
        {
            printf("%d ", s->origin_info.ipv6_addrs[0][i]);
        }
        printf("\n");
    }
    else
    {
        printf("    dom: ");

        for (int i = 0; i < s->origin_info.resolve_addr_len; i++)
        {
            printf("%c", s->origin_info.resolve_addr[i]);
        }
        printf("\n");
    }
    printf("    port: %d%d\n", s->origin_info.port[0], s->origin_info.port[1]);


}

static void
request_init(const unsigned state, struct selector_key *key)
{
    struct request_st *d = &ATTACHMENT(key)->client.request;

    // Adding the read buffer
    d->rb = &(ATTACHMENT(key)->read_buffer);

    // Parser init
    connection_req_parser_init(&d->parser);
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
    size_t count; //Maximum data that can set in the buffer
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
        const enum connection_req_state st = connection_req_consume_message(b, &d->parser, &error);
        //If done parsing and no error
        if (connection_req_done_parsing(&d->parser, &error) && !error){
           ret = request_process(key, d);
        }

        if (error)
        {
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
    if (SELECTOR_SUCCESS != selector_set_interest_key(key, OP_WRITE))
    {
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
    connection_req_parser r_parser = &d->parser;
    // Request information obtained by the parser
    uint8_t cmd = r_parser->finalMessage.cmd;
    uint8_t addr_t = r_parser->socks_5_addr_parser->type;
    uint8_t addr_l = r_parser->socks_5_addr_parser->addrLen;
    uint8_t *addr = r_parser->socks_5_addr_parser->addr;
    uint8_t *dst_port = r_parser->finalMessage.dstPort;

    // We just support connect cmd
    if (cmd == TCP_IP_STREAM)
    {

        // The original quantities of ips stored its 0.
        s->origin_info.ipv4_c = s->origin_info.ipv6_c = 0;

        // Determine the next state depending on the type of address given
        switch (addr_t)
        {
        case IPv4:
            // Save the address
            memcpy(s->origin_info.ipv4_addrs[s->origin_info.ipv4_c++], addr, IP_V4_ADDR_SIZE);
            s->origin_info.ip_selec = IPv4;
            //Save the port.
            memcpy(s->origin_info.port, dst_port, 2);
            // We have all the info to connect
            ret = CONNECTING;
            break;

        case IPv6:
            // Save the address
            memcpy(s->origin_info.ipv6_addrs[s->origin_info.ipv6_c++], addr, IP_V6_ADDR_SIZE);
            s->origin_info.ip_selec = IPv6;
            //Save the port.
            memcpy(s->origin_info.port, dst_port, 2);
            // We have all the info to connect
            ret = CONNECTING;
            break;

        case DOMAIN_NAME:
            // Save the domain name
            s->origin_info.resolve_addr = malloc(addr_l + 1);
            memcpy(s->origin_info.resolve_addr, addr, addr_l);
            s->origin_info.resolve_addr[addr_l] = '\0' ;
            s->origin_info.resolve_addr_len = addr_l;

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
    else
    {
        ret = ERROR;
    }

    return ret;
}

////////////////////////////////////////
// RESOLVE
////////////////////////////////////////

static void
resolve_init(const unsigned state, struct selector_key *key)
{   
    // Resolve state
    struct socks5 * s = ATTACHMENT(key);
    struct resolve_st *r_s = &s->orig.resolve;
    
    // Saving the buffers
    r_s->rb = &(s->read_buffer);
    r_s->wb = &(s->write_buffer);

    //Init the parser
    http_message_parser_init(&r_s->parser);

    /** TODO: MOVE TO WRITE*/
    // Create the socket for the nginx server that will serve as dns.
    struct sockaddr_in serv_addr;
    selector_status st = SELECTOR_SUCCESS;

    char * final_buffer = NULL;
    char * servIP = "127.0.0.1";

    in_port_t servPort = DOH_PORT;
    memset(&serv_addr, 0, sizeof(serv_addr)); // Zero out structure
    serv_addr.sin_family = AF_INET;          // IPv4 address family
    serv_addr.sin_port = htons(servPort);    // Server port

    int portno  = DOH_PORT;

    r_s->doh_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(r_s->doh_fd <= 0){
        perror("ERROR openning socket");
        r_s->doh_fd = -1;
    }
    
    // Connect to nginx server
    if (connect(r_s->doh_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
        perror("ERROR connecting");
        r_s->doh_fd = -1;
    }

    // Register the fd of the nginx server
    st = selector_register(key->s, r_s->doh_fd, &socks5_handler, OP_WRITE, ATTACHMENT(key));
    if (st != SELECTOR_SUCCESS){
        printf("Error registering doh server\n");
        r_s->doh_fd = -1;
    }

    /** TODO: MOVE TO WRITE! */

    // Removing interest from client fd(We dont need to read or write in resolve state)
    if (SELECTOR_SUCCESS != selector_set_interest(key->s, s->client_fd, OP_NOOP)){
        printf("Could not set interest of %d for %d\n", OP_NOOP, s->client_fd);
        /** Todo see what we have to do! */
    }
}

static void
resolve_close(const unsigned state, struct selector_key *key)
{
    // Sock5 state
    struct socks5 *s = ATTACHMENT(key);

    // Reset read and write buffer for reuse.
    buffer_reset(&s->write_buffer);
    buffer_reset(&s->read_buffer);

    /** TODO: Free the http buffer */

}

static unsigned
resolve_process(struct userpass_st *up_s);

/** State when receiving the doh response */
static unsigned
resolve_read(struct selector_key *key)
{
    
    struct socks5 * s = ATTACHMENT(key);
    struct resolve_st *r_s = &s->orig.resolve;
    s->origin_info.ipv4_c = s->origin_info.ipv6_c = 0;

    buffer *b = r_s->rb;
    unsigned ret = RESOLVE;
    int errored = 0;
    uint8_t *ptr;
    ssize_t n;
    size_t count;

    ptr = buffer_write_ptr(b, &count);
    
    n = recv(r_s->doh_fd, ptr, count, MSG_DONTWAIT);
    if(n > 0){

        //doh responds without terminating the body
        ptr[n++] = '\n'; 
        ptr[n++] = '\n';

        //it removes  \r from headers so that parser is consistent
        parse_to_crlf(ptr, &n); 
        // Advancing the buffer
        buffer_write_adv(b, n);

        // Parse JUST the http message and check if done correctly
        http_message_state http_state = http_consume_message(r_s->rb, &r_s->parser, &errored);
        if(http_done_parsing_message(&r_s->parser, &errored) && !errored){
          
            // Parse the dns response and save the info in the origin_info
            parse_dns_resp(r_s->parser.body, s->origin_info.resolve_addr, s, &errored);

            if(s->origin_info.ipv4_c == 0 && s->origin_info.ipv6_c == 0){
                perror("Not valid domain name");
                errored = 1;
            }
            ret = CONNECTING;
        }

    }
    else{
        printf("DOH recv failed\n");
        ret = ERROR;
    }

    // If we are done with parsing and no errors happended -> set to write to clientfd
    if(!errored && ret == CONNECTING){
        // Setting the CLIENT fd for WRITE --> REQUEST WILL need to write
        if (SELECTOR_SUCCESS != selector_set_interest(key->s, s->client_fd, OP_WRITE)){
            ret = ERROR;
        }
    }

    // If we are leaving the resolve state --> unregister the doh fd
    if(errored || ret == CONNECTING){
        // Unregister the dns server fd.
        if(SELECTOR_SUCCESS != selector_unregister_fd(key->s, r_s->doh_fd)){
            ret = ERROR;
        }
    }


    return errored ? ERROR : ret;

}

/** State when sending the doh server the request */
static unsigned
resolve_write(struct selector_key *key)
{
    struct socks5 * s = ATTACHMENT(key);
    struct resolve_st *r_s = &s->orig.resolve;

    unsigned ret = RESOLVE;
    ssize_t n;
    int final_buffer_size = 0;

    // Generate the DNS request
    char * http_request = request_generate(s->origin_info.resolve_addr, &final_buffer_size);

    // Validate that we were able to connect and the request is valid.
    if(r_s->doh_fd > 0 && http_request != NULL){
        // Send the doh request to the nginx server
        n = send(r_s->doh_fd, http_request,final_buffer_size, MSG_DONTWAIT);
        if(n > 0){
            // Set the interests for the selector
            if (SELECTOR_SUCCESS != selector_set_interest(key->s, r_s->doh_fd, OP_READ))
            {
                printf("Could not set interest of %d for %d\n", OP_READ, r_s->doh_fd);
                ret = ERROR;
            }
        }
        else {
            ret = ERROR;
        }
    }
    else{
        ret = ERROR;
    }

    return ret;
}

static unsigned
resolve_process(struct userpass_st *up_s){
    
}

////////////////////////////////////////
// CONNECTING
////////////////////////////////////////

static void determine_connect_error(int error)
{
    switch (error)
    {
    case EACCES:
        printf("The error is EACCES - %d\n", error);
        break;
    case EPERM:
        printf("The error is EPERM - %d\n", error);
        break;
    case EADDRINUSE:
        printf("The error is EADDRINUSE - %d\n", error);
        break;
    case EADDRNOTAVAIL:
        printf("The error is EADDRNOTAVAIL - %d\n", error);
        break;
    case EAFNOSUPPORT:
        printf("The error is EAFNOSUPPORT - %d\n", error);
        break;
    case EAGAIN:
        printf("The error is EAGAIN - %d\n", error);
        break;
    case EALREADY:
        printf("The error is EALREADY - %d\n", error);
        break;
    case EBADF:
        printf("The error is EBADF - %d\n", error);
        break;
    case ECONNREFUSED:
        printf("The error is ECONNREFUSED - %d\n", error);
        break;
    case EFAULT:
        printf("The error is EFAULT - %d\n", error);
        break;
    case EINPROGRESS:
        printf("The error is EINPROGRESS - %d\n", error);
        break;
    case EINTR:
        printf("The error is EINTR - %d\n", error);
        break;
    case EISCONN:
        printf("The error is EISCONN - %d\n", error);
        break;
    case ENETUNREACH:
        printf("The error is ENETUNREACH - %d\n", error);
        break;
    case ENOTSOCK:
        printf("The error is ENOTSOCK - %d\n", error);
        break;
    case EPROTOTYPE:
        printf("The error is EPROTOTYPE - %d\n", error);
        break;
    case ETIMEDOUT:
        printf("The error is ETIMEDOUT - %d\n", error);
        break;
    case EINVAL:
        printf("The error is EINVAL - %d\n", error);
        break;
    }
}

static int connecting_send_conn_response (struct selector_key * key) {
    

    struct connecting_st *d = &ATTACHMENT(key)->orig.conn;
    struct socks5 *s = ATTACHMENT(key);
    struct socks5_origin_info *s5oi = &s->origin_info;
    
    printf("Writing back to client (fd = %d)\n", s->client_fd);
    
    int response_size = 6;
    uint8_t *response = malloc(response_size);
    response[0] = 0x05; // VERSION
    //  STATUS
    if (s->origin_fd < 0)
        response[1] = CONN_RESP_GENERAL_FAILURE;
    else
        response[1] = CONN_RESP_REQ_GRANTED;
    response[2] = 0x00; //RSV
    //BNDADDR
    switch (s5oi->ip_selec)
    {
    case IPv4:
        response_size += IP_V4_ADDR_SIZE;
        response = realloc(response, response_size);
        response[3] = IPv4;
        memcpy(response + 4, s5oi->ipv4_addrs[d->first_working_ip_index], IP_V4_ADDR_SIZE);
        break;
    case IPv6:
        response_size += IP_V6_ADDR_SIZE;
        response = realloc(response, response_size);
        response[3] = IPv6;
        memcpy(response + 4, s5oi->ipv6_addrs[d->first_working_ip_index], IP_V6_ADDR_SIZE);
        break;
    }
    // PORT
    response[response_size - 2] = s5oi->port[0];
    response[response_size - 1] = s5oi->port[1];
    send(s->client_fd, response, response_size, 0);
    free(response);
    return COPY;
}

static int try_connection(int *connect_ret, connecting_st *d, socks5_origin_info *s5oi, AddrType addrType)
{
    int origin_fd = socket(AF_INET, SOCK_STREAM, 0);
    selector_fd_set_nio(origin_fd);
    struct sockaddr_in *sin = (struct sockaddr_in *)&s5oi->origin_addr;
    d->first_working_ip_index = 0;
    do
    {
        // Setting up in socket address
        sin->sin_family = AF_INET;
        memcpy((void *)&sin->sin_addr, s5oi->ipv4_addrs[0], (addrType == IPv4) ? IP_V4_ADDR_SIZE : IP_V6_ADDR_SIZE); // Address
        memcpy((void *)&sin->sin_port, s5oi->port, 2);                                                               // Port
        s5oi->origin_addr_len = sizeof(s5oi->origin_addr);
        *connect_ret = connect(origin_fd, (struct sockaddr *)&s5oi->origin_addr, s5oi->origin_addr_len);
        if (errno == EINPROGRESS || errno == EISCONN || errno == EALREADY)
            return origin_fd;
        if (*connect_ret < 0)
            d->first_working_ip_index++;
    } while (*connect_ret < 0 && d->first_working_ip_index < ((addrType == IPv4) ? s5oi->ipv4_c : s5oi->ipv6_c));
    if (d->first_working_ip_index >= s5oi->ipv4_c)
        d->first_working_ip_index = -1;
    return origin_fd;
}

void connecting_init(const unsigned state, struct selector_key *key)
{
    printf("Connecting Init\n");
}

static unsigned connecting_read(struct selector_key * key) {
    printf("Reading");
    connecting_send_conn_response(key);
    return COPY;
}

static unsigned connecting_write(struct selector_key *key)
{
    // write connection response to client

    struct connecting_st *d = &ATTACHMENT(key)->orig.conn;
    struct socks5 *s = ATTACHMENT(key);
    struct socks5_origin_info *s5oi = &s->origin_info;
    int connect_ret = -1;
    selector_status st = SELECTOR_SUCCESS;
    d->rb = &(ATTACHMENT(key)->read_buffer);

    if (key->fd == s->client_fd) {
        // regular case, try to connect to any of the ips provided
        s->origin_fd = try_connection(&connect_ret, d, s5oi, s5oi->ip_selec);
    } else if (key->fd == s->origin_fd) {
        // this should be entered only when EINPROGRESS is obtained
        // next thing up is to check if connection went well or wrong
        connect_ret = connect(key->fd, (struct sockaddr *)&s5oi->origin_addr, s5oi->origin_addr_len);
        switch (errno) {
            case EISCONN:
            case EALREADY:
                // Connection successful
                // one should reset interests for client_fd (which were set to OP_NOOP)
                // and go to COPY state
                selector_set_interest(key->s, s->client_fd, OP_READ | OP_WRITE);
                // Go on
                connect_ret = -2;
                break;
            default:
                s->origin_fd = -1;
                fprintf(stderr, "Could not connect\n");
                determine_connect_error(errno);
                return ERROR;
        } 
    }

    if (connect_ret == -1)
    {
        // If the error is connection in progress, one needs to register the fd to be able to respond to origin
        switch (errno) {
            case EINPROGRESS:
                printf("I'm waiting connection\n");
                st = selector_register(key->s, s->origin_fd, &socks5_handler, OP_WRITE | OP_READ, ATTACHMENT(key));
                selector_set_interest(key->s, s->client_fd, OP_NOOP);
                if (st != SELECTOR_SUCCESS){
                    printf("Error registering FD to wait for connection\n");
                    s->origin_fd = -1;
                }
                break;
            case EALREADY:
            case EISCONN:
                // Go on
                break;
            default:
                s->origin_fd = -1;
                fprintf(stderr, "Could not connect\n");
                determine_connect_error(errno);
                return ERROR;
        }
    }
    else {
        printf("Connected to origin (fd = %d)\n", s->origin_fd);
        st = selector_register(key->s, s->origin_fd, &socks5_handler, OP_NOOP, ATTACHMENT(key));
        if (st == SELECTOR_SUCCESS)
            printf("Successfully registered origin fd in selector\n");
    }

    //                  ver---status-----------------rsv--
    // struct connecting_st *d = &ATTACHMENT(key)->orig.conn;
    // struct socks5 *s = ATTACHMENT(key);
    // struct socks5_origin_info *s5oi = &s->origin_info;
    // response_size =  1b + 1b + 1b + 1b  + variable + 2b
    // response fields: VER  ST   RSV  TYPE  ADDR       PRT
    
    return connecting_send_conn_response(key);
    // return COPY;
}

void connecting_close(const unsigned state, struct selector_key *key)
{
    printf("Connecting - close\n");
}

////////////////////////////////////////
// COPY
////////////////////////////////////////

static void
copy_init(const unsigned state, struct selector_key *key)
{
    struct socks5 *sockState = ATTACHMENT(key);

    // Init of the copy for the client
    struct copy_st *d = &sockState->client.copy;

    /** TODO: Both copy_st are sharing the same buffer, not sure if ok */

    d->fd = sockState->client_fd;
    d->rb = &sockState->read_buffer;
    d->wb = &sockState->write_buffer;
    d->interest = OP_READ | OP_WRITE;
    d->other_copy = &sockState->orig.copy;

    // Init of the copy for the origin
    d = &sockState->orig.copy;

    d->fd = sockState->origin_fd;
    d->rb = &sockState->write_buffer;
    d->wb = &sockState->read_buffer;
    d->interest = OP_READ | OP_WRITE;
    d->other_copy = &sockState->client.copy;
    // Init request parsers here
    // TODO
}

/**
 * Determines the new interest of the given copy_st and sets it in the selector 
*/
static fd_interest
copy_determine_interests(fd_selector s, struct copy_st *d)
{
    // Basic interest of no operation
    fd_interest interest = OP_NOOP;

    // If the copy_st is interested in reading and we can write in its buffer
    if ((d->interest & OP_READ) && buffer_can_write(d->rb))
    {
        // Add the interest to read
        interest |= OP_READ;
    }

    // If the copy_st is interested in writing and we can read from its buffer
    if ((d->interest & OP_WRITE) && buffer_can_read(d->wb))
    {
        // Add the interest to write
        interest |= OP_WRITE;
    }

    // Set the interests for the selector
    if (SELECTOR_SUCCESS != selector_set_interest(s, d->fd, interest))
    {
        printf("Could not set interest of %d for %d\n", interest, d->fd);
        abort();
    }
    return interest;
}

/**
 *  Gets the pointer to the copy_st depending on the selector fired
 * */
static struct copy_st *
get_copy_ptr(struct selector_key *key)
{
    // Getting the copy struct for the client
    struct copy_st *d = &ATTACHMENT(key)->client.copy;

    // Checking if the selector fired is the client by comparing the fd
    if (d->fd != key->fd)
    {
        d = d->other_copy;
    }

    return d;
}

static unsigned
copy_read(struct selector_key *key)
{
    // Getting the state struct
    struct copy_st *d = get_copy_ptr(key);

    // Getting the read buffer
    buffer *b = d->rb;
    unsigned ret = COPY;
    uint8_t *ptr;
    size_t count; //Maximum data that can set in the buffer
    ssize_t n;

    // Setting the buffer to read
    ptr = buffer_write_ptr(b, &count);
    // Receiving data
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {
        // Notifying the data to the buffer
        buffer_write_adv(b, n);
        // Here analyze the information
        // TODO
    }
    else
    {
        // Closing the socket for reading
        shutdown(d->fd, SHUT_RD);
        // Removing the interest to read from this copy
        d->interest &= ~OP_READ;
        // If the other fd is still open
        if (d->other_copy->fd != -1)
        {
            // Closing the socket for writing
            shutdown(d->other_copy->fd, SHUT_WR);
            // Remove the interest to write
            d->other_copy->interest &= ~OP_WRITE;
        }
    }

    // Determining the new interests for the selectors
    copy_determine_interests(key->s, d);
    copy_determine_interests(key->s, d->other_copy);

    // Checking if the copy_st is not interested anymore in interacting -> Close it
    if (d->interest == OP_NOOP)
    {
        ret = DONE;
    }

    return ret;
}

static unsigned
copy_write(struct selector_key *key)
{
    // Getting the state struct
    struct copy_st *d = get_copy_ptr(key);

    // Getting the read buffer
    buffer *b = d->wb;
    unsigned ret = COPY;
    uint8_t *ptr;
    size_t count; //Maximum data that can set in the buffer
    ssize_t n;

    // Setting the buffer to read
    ptr = buffer_read_ptr(b, &count);

    // Receiving data
    n = send(key->fd, ptr, count, MSG_NOSIGNAL);

    if (n != -1)
    {
        // Notifying the data to the buffer
        buffer_read_adv(b, n);
        // Here analyze the information
        // TODO
    }
    else
    {
        // Closing the socket for writing
        shutdown(d->fd, SHUT_WR);
        // Removing the interest to write from this copy
        d->interest &= ~OP_WRITE;
        // If the other fd is still open
        if (d->other_copy->fd != -1)
        {
            // Closing the socket for reading
            shutdown(d->other_copy->fd, SHUT_RD);
            // Remove the interest for reading
            d->other_copy->interest &= ~OP_READ;
        }
    }

    // Determining the new interests for the selectors
    copy_determine_interests(key->s, d);
    copy_determine_interests(key->s, d->other_copy);

    // Checking if the copy_st is not interested anymore in interacting -> Close it
    if (d->interest == OP_NOOP)
    {
        ret = DONE;
    }

    return ret;
}

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
        .on_write_ready = copy_write},
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
                printf("Socks is done for %d\n", fds[i]);
                abort();
            }
            close(fds[i]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// SOCKS5 ORIGIN HANDLERS
////////////////////////////////////////////////////////////////////////////////

static void socks5_origin_read(struct selector_key *key) {
    // TODO implement
    printf("ORIGIN READ: UNIMPLEMENTED\n");
}
static void socks5_origin_write(struct selector_key *key) {
    // TODO implement
    printf("ORIGIN WRITE: UNIMPLEMENTED\n");
}
static void socks5_origin_close(struct selector_key *key) {
    // TODO implement
    printf("ORIGIN CLOSE: UNIMPLEMENTED\n");
}
static void socks5_origin_block(struct selector_key *key) {
    // TODO implement
    printf("ORIGIN BLOCK: UNIMPLEMENTED\n");
}