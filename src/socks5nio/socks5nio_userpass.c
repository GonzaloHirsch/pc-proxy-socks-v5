#include "socks5nio/socks5nio_userpass.h"

////////////////////////////////////////
// USERPASS_READ
////////////////////////////////////////

void
userpass_read_init(const unsigned state, struct selector_key *key)
{
    struct socks5 *s = ATTACHMENT(key);

    struct userpass_st * up_s= &s->client.userpass;

    up_s->rb = &(s->read_buffer);
    up_s->wb = &(s->write_buffer);
    up_req_parser_init(&up_s->parser);
}

void
userpass_read_close(const unsigned state, struct selector_key *key)
{
    struct socks5 *sock_state = ATTACHMENT(key);

    free_up_req_parser(&sock_state->client.userpass.parser);
}

static unsigned
userpass_process(struct userpass_st *up_s, bool * auth_valid);

unsigned
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
        // Metrics
        add_transfered_bytes(n);
        
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
    uint8_t * uid = up_s->parser.uid;
    uint8_t * pw = up_s->parser.pw;
    uint8_t uid_l = up_s->parser.uidLen;

    *auth_valid = validate_user_proxy(uid, pw);
    
    if(*auth_valid){            
        up_s->user = malloc(uid_l + 1);
        memcpy(up_s->user, uid, uid_l);
        up_s->user[uid_l] = 0x00;
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

void
userpass_write_init(const unsigned state, struct selector_key *key)
{
}

void
userpass_write_close(const unsigned state, struct selector_key *key)
{
    struct socks5 *sock_state = ATTACHMENT(key);

    // Reset read and write buffer for reuse.
    buffer_reset(&sock_state->write_buffer);
    buffer_reset(&sock_state->read_buffer);

    /** TODO: Free memory of userpass_st */   
}

unsigned
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

    // Check if the authentication was successful if not --> Error
    if (up_response[1] != AUTH_SUCCESS)
    {
        ret = ERROR;
    }

    return ret;
}