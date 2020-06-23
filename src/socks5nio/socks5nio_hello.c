#include "socks5nio/socks5nio_hello.h"

////////////////////////////////////////
// HELLO
////////////////////////////////////////

/** inicializa las variables de los estados HELLO_… */
void
hello_read_init(const unsigned state, struct selector_key *key)
{
    struct hello_st *d = &ATTACHMENT(key)->client.hello;

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->wb = &(ATTACHMENT(key)->write_buffer);
    hello_parser_init(&d->parser);
}

void
hello_read_close(const unsigned state, struct selector_key *key)
{
   struct socks5 *sock_state = ATTACHMENT(key);

   free_hello_parser(&sock_state->client.hello.parser);
}

static unsigned
hello_process(struct hello_st *d);

/** lee todos los bytes del mensaje de tipo `hello' y inicia su proceso */
unsigned
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
        // Metrics
        add_transfered_bytes(n);
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
hello_process(struct hello_st *d)
{
    unsigned ret = HELLO_WRITE;
    uint8_t *methods = d->parser.auth;
    uint8_t methods_c = d->parser.nauth;
    uint8_t m = SOCKS_HELLO_NO_ACCEPTABLE_METHODS;


    for (int i = 0; i < methods_c; i++)
    {    
        if(methods[i] == SOCKS_HELLO_USERPASS){
            m = SOCKS_HELLO_USERPASS;
            break;
        }
    }

    // Save the method.
    d->method = m;

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


void
hello_write_init(const unsigned state, struct selector_key *key)
{
}

void
hello_write_close(const unsigned state, struct selector_key *key)
{

    struct socks5 *sock_state = ATTACHMENT(key);

    // Reset read and write buffer for reuse.
    buffer_reset(&sock_state->write_buffer);
    buffer_reset(&sock_state->read_buffer);

    /** TODO: Free memory of hello_st */
}

/** lee todos los bytes del mensaje de tipo `hello' y inicia su proceso */
unsigned
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
        // Metrics
        add_transfered_bytes(n);
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
    if(auth == SOCKS_HELLO_USERPASS){
         ret = USERPASS_READ;
    }
   else{
       ret = ERROR;
   }

    return ret;
}