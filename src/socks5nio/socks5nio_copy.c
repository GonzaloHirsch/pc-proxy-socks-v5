#include "socks5nio/socks5nio_copy.h"

////////////////////////////////////////
// COPY
////////////////////////////////////////

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

unsigned
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