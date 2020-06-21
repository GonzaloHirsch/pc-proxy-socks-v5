#include "socks5nio/socks5nio_request.h"

////////////////////////////////////////
// REQUEST
////////////////////////////////////////
unsigned int identify_protocol_type(uint8_t * port);

/** Frees the parser used */
void
request_close(const unsigned state, struct selector_key *key)
{
    // Sock5 state
    struct socks5 *s = ATTACHMENT(key);

    // Sends reply failure if needed
    if(s->reply_type != -1){
        send_reply_failure(key);
    }

    // Reset read and write buffer for reuse.
    buffer_reset(&s->write_buffer);
    buffer_reset(&s->read_buffer);

    // Freeing the parser
    free_connection_req_parser(&s->client.request.parser);

    /** TODO: Free everything */

    // All temporal for testing... -----> DELTE SHORTLY

/*
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
*/

}

void
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

unsigned
request_read(struct selector_key *key)
{   
    struct socks5 *s= ATTACHMENT(key);
    // Getting the state struct
    struct request_st *d = &s->client.request;

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
        // Metrics
        add_transfered_bytes(n);
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
            switch (st)
            {
            case CONN_REQ_ERR_INV_VERSION:
            case CONN_REQ_ERR_INV_RSV:
            case CONN_REQ_GENERIC_ERR:
                /** TODO: see if there is a better response to this states */
                s->reply_type = REPLY_RESP_GENERAL_FAILURE; 
                break;
            case CONN_REQ_ERR_INV_DSTADDR:
                s->reply_type = REPLY_RESP_ADDR_TYPE_NOT_SUPPORTED;
                break;
            case CONN_REQ_ERR_INV_CMD:
                s->reply_type = REPLY_RESP_CMD_NOT_SUPPORTED;
                break;
            default:
                break;
            }
            ret = ERROR;
        }
    }
    else
    {
        s->reply_type=REPLY_RESP_GENERAL_FAILURE;
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
            s->origin_info.protocol_type = identify_protocol_type(dst_port);
            // We have all the info to connect
            ret = CONNECTING;
            break;

        case IPv6:
            // Save the address
            memcpy(s->origin_info.ipv6_addrs[s->origin_info.ipv6_c++], addr, IP_V6_ADDR_SIZE);
            s->origin_info.ip_selec = IPv6;
            //Save the port.
            memcpy(s->origin_info.port, dst_port, 2);
            s->origin_info.protocol_type = identify_protocol_type(dst_port);
            //State that we have no domain
            s->origin_resolution = NULL;
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
            s->origin_info.protocol_type = identify_protocol_type(dst_port);

            // We need to resolve the domain name.
            ret = RESOLVE;
            break;

        default:
            s->reply_type = REPLY_RESP_ADDR_TYPE_NOT_SUPPORTED;
            ret = ERROR;
            break;
        }
    }
    else
    {
        s->reply_type = REPLY_RESP_CMD_NOT_SUPPORTED; 
        ret = ERROR;
    }

    return ret;
}

unsigned int
identify_protocol_type(uint8_t * port){

    if(port == NULL){
        return PROT_UNIDENTIFIED;
    }
    else if(port[0] == 0 && port[1] == 80){
        return PROT_HTTP;
    }
    else if(port[0] == 0 && port[1] == 110){
        return PROT_POP3;
    }

    return PROT_UNIDENTIFIED;
}