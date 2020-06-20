#include "socks5nio/socks5nio_resolve.h"


////////////////////////////////////////
// RESOLVE
////////////////////////////////////////

static unsigned int
doh_check_connection(struct resolve_st *r_s)
{   
    int connect_ret;

    connect_ret = connect(r_s->doh_fd, (struct sockaddr *) &r_s->serv_addr, sizeof(r_s->serv_addr));
    if (connect_ret < 0) {
        switch (errno) {
            case EINPROGRESS:
            case EALREADY:
               return CONN_INPROGRESS;
            case EISCONN:
                return CONN_SUCCESS;
                break;
            default:
                determine_connect_error(errno);
                return CONN_FAILURE;        
        }
    }
    else {
        return CONN_SUCCESS;
    }
}

void
resolve_init(const unsigned state, struct selector_key *key)
{   
    // Resolve state
    struct socks5 * s = ATTACHMENT(key);
    struct resolve_st *r_s = &s->orig.resolve;
    
    // Saving the buffers
    r_s->rb = &(s->read_buffer);
    r_s->wb = &(s->write_buffer);
 
    //Init parsers
    http_message_parser_init(&r_s->parser);

    int connect_ret;
    selector_status st = SELECTOR_SUCCESS;

    // Structure to connect to the dns server
 
    memset(&r_s->serv_addr, 0, sizeof(r_s->serv_addr));              // Zero out structure
    r_s->serv_addr.sin_family = AF_INET;                             // IPv4 address family
    r_s->serv_addr.sin_addr.s_addr = inet_addr(options ->doh.ip);            // Address
    r_s->serv_addr.sin_port = htons(options -> doh.port);                       // Server port


    // Creating the fd for the ipv4 doh connection
    r_s->doh_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(r_s->doh_fd <= 0){
        printf("ERROR openning socket for ipv4\n");
        r_s->doh_fd = -1;
    }
    // Set as non blocking
    selector_fd_set_nio(r_s->doh_fd);
    // Trying connection
    connect_ret = connect(r_s->doh_fd, (struct sockaddr *) &r_s->serv_addr, sizeof(r_s->serv_addr));

    // If we were able to connect or we are still connecting
    if (connect_ret >= 0 || (connect_ret == -1 && errno == EINPROGRESS)) {
       // Register the fd of the nginx server
        st = selector_register(key->s, r_s->doh_fd, &socks5_handler, OP_WRITE, ATTACHMENT(key));
        if (st != SELECTOR_SUCCESS){
            printf("Error registering doh server\n");
            r_s->doh_fd = -1;
        }
        // Unregistering client interests
        st = selector_set_interest(key->s, s->client_fd, OP_NOOP);
        if(st != SELECTOR_SUCCESS){
            printf("Failure setting interest for client\n");
            /** TODO: handle error */
        }
        // Determine if we already connected or in progress
        if(connect_ret >= 0){
            r_s->conn_state = CONN_SUCCESS;
        }
        else{
            r_s->conn_state = CONN_INPROGRESS;
        }
    }
    else{
        r_s->doh_fd = -1;
        fprintf(stderr, "Could not connect to doh server\n");
        determine_connect_error(errno);
        r_s->conn_state = CONN_FAILURE;
    }

}



void
resolve_close(const unsigned state, struct selector_key *key)
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

    // Unregister the doh_fd.
    selector_unregister_fd(key->s, s->orig.resolve.doh_fd);

    // Free http parser
    free_http_message_parser(&s->orig.resolve.parser);
}

// static unsigned
// resolve_process(struct userpass_st *up_s);

/** State when receiving the doh response */
unsigned
resolve_read(struct selector_key *key)
{
    
    struct socks5 * s = ATTACHMENT(key);
    struct resolve_st *r_s = &s->orig.resolve;

    buffer *b = r_s->rb;
    unsigned ret = RESOLVE;
    int errored = 0;
    uint8_t *ptr;
    size_t n;
    size_t count;

    ptr = buffer_write_ptr(b, &count);
    
    n = recv(r_s->doh_fd, ptr, count, MSG_DONTWAIT);
    if(n > 0){
        // Advancing the buffer
        buffer_write_adv(b, n);

        // We are going to receive the responses in order.
        switch (r_s->resp_state)
        {
        // In case we havent start parsing or we are parsing the ipv4 response
        case RES_RESP_IPV4:
            // Consume the message
            http_consume_message(r_s->rb, &r_s->parser, &errored);

            //If we done parsing the first response
            if(http_done_parsing_message(&r_s->parser, &errored) && strcmp((const char *) r_s->parser.status, "200") == 0){ //checks if it is done the http parsing and that the status code is not error
                
                // If the http message is correct --> parse the dns response
                if(!errored){
                    // Parse the dns response and save the info in the origin_info
                    parse_dns_resp(r_s->parser.body, (char *) s->origin_info.resolve_addr, s, &errored);
                }
                else{
                    /** TODO: See if we can santize the rest of the http message 
                     * to leave it ready for the next parsing */
                }

                // Reset the parser for the ipv6 response
                http_message_parser_init(&r_s->parser);
                
                r_s->resp_state = RES_RESP_IPV6;
                //Notice: Dont break so we fall into Ipv6 state to check if there is something to read.
            }
            else{
                // Break, still need to finish parsing the first response.
                break;
            }
        
        case RES_RESP_IPV6:

            // Consume the message
            http_consume_message(r_s->rb, &r_s->parser, &errored);

            //If we done parsing the first response
            if(http_done_parsing_message(&r_s->parser, &errored)  && strcmp((const char *) r_s->parser.status, "200") == 0){
                
                // If the http message is correct --> parse the dns response
                if(!errored){
                    // Parse the dns response and save the info in the origin_info
                    parse_dns_resp(r_s->parser.body, (char *) s->origin_info.resolve_addr, s, &errored);
                }
                else{
                    
                }
                r_s->resp_state = RES_RESP_DONE;
            }
            else{
                // Still needs to parse the second response
            }

            break;
        case RES_RESP_DONE:
            // Shouldnt happen, just in case
            break;
        default:
            s->reply_type = REPLY_RESP_GENERAL_FAILURE;
            ret = ERROR;
        }
    }
    // If connection was closed --> We are done and need to check if we collected a ip
    else if(n == 0){
       r_s->resp_state = RES_RESP_DONE;
    }
    else{
        s->reply_type = REPLY_RESP_HOST_UNREACHABLE;
        ret = ERROR;
    }

    // If already recevied both responses.
    if(r_s->resp_state == RES_RESP_DONE){

        // If at least one ip was recevied correctly
        if(s->origin_info.ipv4_c > 0 || s->origin_info.ipv6_c > 0){

            //Saving the ip preferred and the state
            s->origin_info.ip_selec = (s->origin_info.ipv4_c > 0) ? IPv4 : IPv6;
            ret = CONNECTING;

            // Setting the CLIENT fd for WRITE --> REQUEST WILL need to write
            if (SELECTOR_SUCCESS != selector_set_interest(key->s, s->client_fd, OP_WRITE)){
                s->reply_type = REPLY_RESP_GENERAL_FAILURE;
                ret = ERROR;
            }
        }
        else {
            s->reply_type = REPLY_RESP_HOST_UNREACHABLE;
            ret = ERROR;
        }    
    }

    return ret;

}

/** State when sending the doh server the request */
unsigned
resolve_write(struct selector_key *key)
{
    struct socks5 * s = ATTACHMENT(key);
    struct resolve_st *r_s = &s->orig.resolve;
    //Establish no ips(in case fo error)
    s->origin_info.ipv4_c = s->origin_info.ipv6_c = 0;

    unsigned ret = RESOLVE;
    ssize_t n, m;
    int final_buffer_size = 0, final_buffer_size2 = 0;

    // If connecting in progress, check if connection done.
    if(r_s->conn_state == CONN_INPROGRESS){
         r_s->conn_state = doh_check_connection(r_s);
    }

    switch (r_s->conn_state)
    {
    // If still in progress try back later.
    case CONN_INPROGRESS:
        return RESOLVE;
        break;
    case CONN_FAILURE:
        // Establish the reply type for the reply to the cleint
        s->reply_type = REPLY_RESP_NET_UNREACHABLE;
        return ERROR;
    //The connection has been established :)
    case CONN_SUCCESS:
        break;
    default:
        s->reply_type = REPLY_RESP_GENERAL_FAILURE;
        return ERROR;
    }

    // Generate the DNS requests: First the ipv4 then th
    char * http_request = request_generate((char *)s->origin_info.resolve_addr, &final_buffer_size, T_A); //request ipv4
    char * http_request2 = request_generate((char *)s->origin_info.resolve_addr, &final_buffer_size2, T_AAAA); //request ipv6

    // Validate that we were able to connect and the request is valid.
    if(r_s->doh_fd > 0 && http_request != NULL && http_request2 != NULL){
        // Send the doh request to the nginx server
        n = send(r_s->doh_fd, http_request,final_buffer_size, MSG_DONTWAIT);
        m = send(r_s->doh_fd, http_request2,final_buffer_size2, MSG_DONTWAIT);
        if(n > 0 && m > 0){
            // Establish the resolve response state.
            r_s->resp_state = RES_RESP_IPV4;
            s->origin_info.ipv4_c = s->origin_info.ipv6_c = 0;

            // Set the interests for the selector
            if (SELECTOR_SUCCESS != selector_set_interest(key->s, r_s->doh_fd, OP_READ))
            {
                ret = ERROR;
            }
        }
        else {
            s->reply_type=REPLY_RESP_GENERAL_FAILURE;
            ret = ERROR;
        }
    }
    else{
        s->reply_type=REPLY_RESP_GENERAL_FAILURE;
        ret = ERROR;
    }

    // Free the http_request message
    free(http_request); 
    free(http_request2);

    return ret;
}