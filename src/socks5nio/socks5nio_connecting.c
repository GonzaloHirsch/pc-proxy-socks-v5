#include "socks5nio/socks5nio_connecting.h"

////////////////////////////////////////
// CONNECTING
////////////////////////////////////////

//                  ver---status-----------------rsv--
// struct connecting_st *d = &ATTACHMENT(key)->orig.conn;
// struct socks5 *s = ATTACHMENT(key);
// struct socks5_origin_info *s5oi = &s->origin_info;
// response_size =  1b + 1b + 1b + 1b  + variable + 2b
// response fields: VER  ST   RSV  TYPE  ADDR       PRT
static int connecting_send_conn_response (struct selector_key * key) {
    struct connecting_st *d = &ATTACHMENT(key)->orig.conn;
    struct socks5 *s = ATTACHMENT(key);
    struct socks5_origin_info *s5oi = &s->origin_info;
    
    int response_size = 6;
    uint8_t *response = malloc(response_size);
    response[0] = 0x05; // VERSION
    //  STATUS
    if (s->sel_origin_fd < 0)
        response[1] = REPLY_RESP_REFUSED_BY_DEST_HOST;
    else
        response[1] = REPLY_RESP_SUCCESS;
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
    // TODO react to error

    // Logging the request
    log_request(0, s->username, (const struct sockaddr *)&s->client_addr, (const struct sockaddr *)&s->origin_info.origin_addr, s->origin_info.resolve_addr);
    
    selector_set_interest(key->s, s->sel_origin_fd, OP_READ | OP_WRITE);
    selector_set_interest(key->s, s->client_fd, OP_READ | OP_WRITE);
    return COPY;
}

static int try_connection(int origin_fd, int *connect_ret, struct selector_key * key, AddrType addrType)
{
    struct connecting_st *d = &ATTACHMENT(key)->orig.conn;
    struct socks5 *s = ATTACHMENT(key);
    struct socks5_origin_info *s5oi = &s->origin_info;

    struct sockaddr_storage *sin = (struct sockaddr_storage *)&s5oi->origin_addr;
    
    while (*connect_ret < 0 && d->first_working_ip_index < ((addrType == IPv4) ? s5oi->ipv4_c : s5oi->ipv6_c))
    {
        // Setting up in socket address
        if (addrType == IPv4) {
            sin->ss_family = AF_INET;
            memcpy((void *)&(((struct sockaddr_in*)sin)->sin_addr), s5oi->ipv4_addrs[d->first_working_ip_index], IP_V4_ADDR_SIZE); // Address
            memcpy((void *)&(((struct sockaddr_in*)sin)->sin_port), s5oi->port, 2);
        }
        else if (addrType == IPv6) {
            printf("IPV6\n");
            sin->ss_family = AF_INET6;
            memcpy((void *)&(((struct sockaddr_in6*)sin)->sin6_addr), s5oi->ipv6_addrs[d->first_working_ip_index], IP_V6_ADDR_SIZE); // Address
            memcpy((void *)&(((struct sockaddr_in6*)sin)->sin6_port), s5oi->port, 2); // Port
        }
        s5oi->origin_addr_len = sizeof(s5oi->origin_addr);
        *connect_ret = connect(origin_fd, (struct sockaddr *)&s5oi->origin_addr, s5oi->origin_addr_len);
        if (errno == EINPROGRESS) {
            return origin_fd;
        }
        if (*connect_ret < 0) {
            // Does it even get to this case?
            d->first_working_ip_index++;
        }
    } 
    // If all addresses have been checked, then one has to check if all the addreses for the other family have also been
    // checked
    if (d->first_working_ip_index >= ((addrType == IPv4) ? s5oi->ipv4_c : s5oi->ipv6_c)) {
        if (d->families_to_check == 1) {
            d->families_to_check = 0;
            d->first_working_ip_index = -1;
        }
        else if (d->families_to_check == 2) {
            d->first_working_ip_index = 0;
            d->families_to_check = 1;
            s5oi->ip_selec = (addrType == IPv4) ? IPv6 : IPv4;
            s->sel_origin_fd = (addrType == IPv4) ? s->origin_fd6 : s->origin_fd;
        }
    }
    return origin_fd;
}

void connecting_init(const unsigned state, struct selector_key *key)
{
    
    struct connecting_st *d = &ATTACHMENT(key)->orig.conn;
    struct socks5 *s = ATTACHMENT(key);
    struct socks5_origin_info *s5oi = &s->origin_info;
    int connect_ret = -1;
    selector_status st = SELECTOR_SUCCESS;
    if (s->origin_info.ipv4_c > 0)
        s->origin_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s->origin_info.ipv6_c > 0)
        s->origin_fd6 = socket(AF_INET6, SOCK_STREAM, 0);
    int no = 0;
    setsockopt(s->origin_fd6, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no));
    selector_fd_set_nio(s->origin_fd);
    selector_fd_set_nio(s->origin_fd6);

    d->rb = &(ATTACHMENT(key)->read_buffer);
    d->first_working_ip_index = 0;
    // Need to know how many families there are to check
    d->families_to_check += s->origin_info.ipv4_c > 0;
    d->families_to_check += s->origin_info.ipv6_c > 0;
    d->substate = CONN_SUB_TRY_IPS;

    // Attempt to connect with addresses of the selected family
    s->sel_origin_fd = (s5oi->ip_selec == IPv4) ? s->origin_fd : s->origin_fd6;
    try_connection(s->sel_origin_fd, &connect_ret, key, s5oi->ip_selec);

    if (connect_ret == -1)
    {
        // If the error is connection in progress, one needs to register the fd to be able to respond to origin
        switch (errno) {
            case EINPROGRESS:
                st = selector_register(key->s, s->sel_origin_fd, &socks5_handler, OP_WRITE, ATTACHMENT(key));
                st = selector_set_interest(key->s, s->client_fd, OP_NOOP);
                if (st != SELECTOR_SUCCESS){
                    // s->origin_fd = -1;
                    return;
                }
                // On next write handle, connecting to origin must be checked
                d->substate = CONN_SUB_CHECK_ORIGIN;
                break;
            default:
                // If this is reached, then every ip has been tried for preferred family
                // SHOULD try with the other family (TODO)
                // s->origin_fd = -1;
                d->first_working_ip_index = 0;
                // determine_connect_error(errno);
                if (d->families_to_check > 0) {
                    d->substate = CONN_SUB_TRY_IPS;
                }
                else {
                    d->substate = CONN_SUB_ERROR;
                }
        }
    }
    if (connect_ret > 0) {
        st = selector_register(key->s, s->sel_origin_fd, &socks5_handler, OP_NOOP, ATTACHMENT(key));
        if (st == SELECTOR_SUCCESS){
        }
    }

}

static unsigned try_ips(struct selector_key * key) {
    struct socks5 * s = ATTACHMENT(key);
    struct connecting_st * d = &s->orig.conn;
    int connect_ret = -1;
    try_connection(s->sel_origin_fd, &connect_ret, key, s->origin_info.ip_selec);
        if (connect_ret < 0) {
            switch (errno) {
                // TODO try to remove code repetition
                case EINPROGRESS:
                    // selector_status st = selector_register(key->s, s->sel_origin_fd, &socks5_handler, OP_WRITE, ATTACHMENT(key));
                    // if (st != SELECTOR_SUCCESS){
                    //     s->sel_origin_fd = -1;
                    //     return;
                    // }
                    d->substate = CONN_SUB_CHECK_ORIGIN;
                    return CONNECTING;
                    break;
                default:
                    // s->sel_origin_fd = -1;
                    if (d->first_working_ip_index >= ((s->origin_info.ip_selec == IPv4) ?  s->origin_info.ipv4_c : s->origin_info.ipv6_c) &&
                        d->families_to_check > 0) {
                        // TODO Actually, here, one must check if all options for
                        // both or only one family have been tried (and try with the other family)
                            d->substate = CONN_SUB_TRY_IPS;
                            // Start from first address
                            d->first_working_ip_index = 0;
                            return try_ips(key);
                        }
                    else if (d->first_working_ip_index < ((s->origin_info.ip_selec == IPv4) ?  s->origin_info.ipv4_c : s->origin_info.ipv6_c)) {
                        d->first_working_ip_index++;
                        return try_ips(key);
                    }
                    determine_connect_error(errno);
                    s->reply_type = REPLY_RESP_REFUSED_BY_DEST_HOST;
                    return ERROR;
            }
        }
        // Connected, just follow the usual routine
        else {
            selector_set_interest(key->s, s->client_fd, OP_READ | OP_WRITE);
            return connecting_send_conn_response(key);
        }
}

static unsigned connecting_check_origin_connected(struct selector_key * key) {
    struct socks5 * s = ATTACHMENT(key);
    struct connecting_st * d = &s->orig.conn;
    int connect_ret = connect(key->fd, (struct sockaddr *)&s->origin_info.origin_addr, s->origin_info.origin_addr_len);
    if (connect_ret < 0) {
        switch (errno) {
            case EISCONN:
            case EINPROGRESS:
            case EALREADY:
                // All harmless errors but shouldn't happen
                // TODO decide how they should be handled
                return CONNECTING;
            default:
                // s->sel_origin_fd = -1;
                // Connection progress ended up in error: try with 
                if (d->first_working_ip_index >= ((s->origin_info.ip_selec == IPv4) ?  s->origin_info.ipv4_c : s->origin_info.ipv6_c) &&
                    d->families_to_check > 0) {
                // TODO Actually, here, one must check if all options for
                // both or only one family have been tried (and try with the other family)
                    d->substate = CONN_SUB_TRY_IPS;
                    d->first_working_ip_index = 0;
                    return try_ips(key);
                }
                else if (d->first_working_ip_index < ((s->origin_info.ip_selec == IPv4) ?  s->origin_info.ipv4_c : s->origin_info.ipv6_c)) {
                    d->first_working_ip_index++;
                    return try_ips(key);
                }
                determine_connect_error(errno);
                s->reply_type = REPLY_RESP_REFUSED_BY_DEST_HOST;
                return ERROR;
        }
    }
    else {
        // Connected after EINPROGRESS. All done: set client_fd for reading 
        // and writing again, then send response and go to COPY
        selector_set_interest(key->s, s->client_fd, OP_READ | OP_WRITE);
        return connecting_send_conn_response(key);
    }
}

// not considered for now
unsigned connecting_read(struct selector_key * key) {
    if (key->fd == ATTACHMENT(key)->origin_fd) {
        return connecting_check_origin_connected(key);
    }
    // shouldn't reach this spot
    ATTACHMENT(key)->reply_type = REPLY_RESP_REFUSED_BY_DEST_HOST;
    return ERROR;
}


unsigned connecting_write(struct selector_key *key)
{
    struct connecting_st * d = &ATTACHMENT(key)->orig.conn;
    struct socks5 *s = ATTACHMENT(key);
    if (key->fd == s->client_fd) {
        // shouldn't happen
        return connecting_send_conn_response(key);
    } else if (key->fd == s->sel_origin_fd) {
        // this should be entered only when EINPROGRESS is obtained
        // next thing up is to check if connection went well or wrong
        switch (d->substate) {
            case CONN_SUB_CHECK_ORIGIN:
                return connecting_check_origin_connected(key);
                break;
            case CONN_SUB_TRY_IPS:
                return try_ips(key);
            case CONN_SUB_ERROR:
                s->reply_type = REPLY_RESP_REFUSED_BY_DEST_HOST;
                return ERROR;

        }
    }
    // shouldn't reach this spot
    s->reply_type = REPLY_RESP_REFUSED_BY_DEST_HOST;
    return ERROR;
}

void connecting_close(const unsigned state, struct selector_key *key)
{
    struct socks5 *s = ATTACHMENT(key);

    // Sends reply failure if needed
    if(s->reply_type != -1){
        send_reply_failure(key);
    }

    /** TODO: FREE MEMORY */
}