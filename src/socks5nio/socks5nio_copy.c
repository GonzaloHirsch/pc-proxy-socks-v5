#include "socks5nio/socks5nio_copy.h"

////////////////////////////////////////
// COPY
////////////////////////////////////////
void extract_http_auth(struct http_auth_parser * http_p);
void extract_pop3_auth(pop3_parser pop3_p);

void
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
    // Init parser and buffer, just for the client fd
    http_auth_init(&d->http_parser);
    pop3_parser_init(&d->pop_parser);
    buffer_init(&d->aux_b, BUFFERSIZE+1, malloc(BUFFERSIZE+1));


    // Init of the copy for the origin
    d = &sockState->orig.copy;

    d->fd = sockState->sel_origin_fd;
    d->rb = &sockState->write_buffer;
    d->wb = &sockState->read_buffer;
    d->interest = OP_READ | OP_WRITE;
    d->other_copy = &sockState->client.copy;
}

void
copy_close(const unsigned state, struct selector_key *key)
{
    struct socks5 *s = ATTACHMENT(key);

    free_http_auth_parser(&s->client.copy.http_parser);
    free(s->client.copy.aux_b.data);
    /** TODO: Free remaining stuff */
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

unsigned
copy_read(struct selector_key *key)
{
    struct socks5 * s = ATTACHMENT(key);
    // Getting the state struct
    struct copy_st *d = get_copy_ptr(key);

    // Getting the read buffer
    buffer *b = d->rb;
    unsigned ret = COPY;
    uint8_t *ptr;
    size_t count; //Maximum data that can set in the buffer
    ssize_t n;
    int errored = 0;
    buffer * aux_b;

    // Setting the buffer to read
    ptr = buffer_write_ptr(b, &count);
    // Receiving data
    n = recv(key->fd, ptr, count, 0);
    if (n > 0)
    {
        // Metrics
        add_transfered_bytes(n);
        // Notifying the data to the buffer
        buffer_write_adv(b, n);

        // Analysing the information just for the client
        if(key->fd == s->client_fd){

            // Temporary buffer so we dont override the other buffer
            aux_b = &d->aux_b;
            buffer_reset(aux_b);
            memcpy(aux_b->data, ptr, n);
            buffer_write_adv(aux_b, n);
            
            switch (s->origin_info.protocol_type)
            {
            case PROT_HTTP:
                // Analyze the whole message to steal 1 or more passwords
                while(buffer_can_read(aux_b)){
                    http_auth_consume_msg(aux_b, &d->http_parser, &errored);
                    if(http_auth_done_parsing(&d->http_parser, &errored)){
                        if(!errored){
                            extract_http_auth(&d->http_parser);
                        }
                        free_http_auth_parser(&d->http_parser);
                        http_auth_init(&d->http_parser);
                    }
                }

                break;
            case PROT_POP3:
                while(buffer_can_read(aux_b)){
                    pop3_consume_msg(aux_b, &d->pop_parser, &errored);
                    if(pop3_done_parsing(&d->pop_parser, &errored)){
                        if(!errored){
                            extract_pop3_auth(&d->pop_parser);
                        }
                        free_pop3_parser(&d->pop_parser);
                        pop3_parser_init(&d->pop_parser);
                    }
                    //buffer_read(aux_b);
                }
                break;
            default:
                break;
            }
            
        }
        else{

        }
       
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

unsigned
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
        // Metrics
        add_transfered_bytes(n);
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

void
extract_http_auth(struct http_auth_parser * http_p){
    

}

void
extract_pop3_auth(pop3_parser pop3_p){
    if(pop3_p->user != NULL && pop3_p->pass != NULL){
        printf("User: %s Pass: %s\n", pop3_p->user, pop3_p->pass);
    }
}