#include "parsers/connection_req_parser.h"
#include "parsers/socks_5_addr_parser.h"

typedef enum CMDType {
    TCP_IP_STREAM = 0x01,
    TCP_IP_PORT_BINDING = 0x02,
    UDP = 0x03
} CMDType;


connection_req_parser new_connection_req_parser() {
    connection_req_parser crp = malloc(sizeof(struct connection_req_parser));
    crp->socks_5_addr_parser = NULL;
    return crp;
}

enum connection_req_state connection_req_read_next_byte(connection_req_parser p, const uint8_t b) {
    switch (p->state) {
        case CONN_REQ_VERSION:
            if (b == 0x05) {
                p->finalMessage.ver = b;
                p->state = CONN_REQ_CMD;
            }
            else {
                p->state = CONN_REQ_ERR_INV_VERSION;
            }
            break;
        case CONN_REQ_CMD:
            if (b == TCP_IP_STREAM ||
                b == TCP_IP_PORT_BINDING ||
                b == UDP) {
                    p->finalMessage.cmd = b;
                    p->state = CONN_REQ_RSV;
                }
            else 
                p->state = CONN_REQ_ERR_INV_CMD;
            break;
        case CONN_REQ_RSV:
            if (b == 0) {
                p->finalMessage.rsv = 0;
                p->state = CONN_REQ_DSTADDR;
                p->bytes_to_read = 2; // Untidy, this is for BND_PORT
            }
            else 
                p->state = CONN_REQ_ERR_INV_RSV;
            break;
        case CONN_REQ_DSTADDR:
            // this state is just for consume function
            // to know when to use socks_5_addr_parser
            break;
        case CONN_REQ_BND_PORT:
            if (p->bytes_to_read) {
                p->finalMessage.dstPort[2-p->bytes_to_read] = b; //TODO do sth about magic number
                p->bytes_to_read--;
            }
            else
                p->state = CONN_REQ_DONE;            
            break;
        case CONN_REQ_DONE:

            break;
        case CONN_REQ_ERR_INV_VERSION:
            
            break;
        case CONN_REQ_ERR_INV_CMD:
            
            break;
        case CONN_REQ_ERR_INV_RSV:
            
            break;
        case CONN_REQ_ERR_INV_DSTADDR:
            
            break;
        case CONN_REQ_GENERIC_ERR:
            
            break;
        }
    return p->state;
}
enum connection_req_state connection_req_consume_message(buffer * b, connection_req_parser p, int *errored) {
    connection_req_state st = p->state;
    while(buffer_can_read(b) && p->state != CONN_REQ_DSTADDR && !connection_req_done_parsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = connection_req_read_next_byte(p, c);
    }
    // Reading Address Part...
    p->socks_5_addr_parser = new_socks_5_addr_parser();
    socks_5_addr_state s5st = socks_5_addr_consume_message(b, p->socks_5_addr_parser, errored);
    if (s5st > SOCKS5ADDR_DONE) {
        st = p->state = CONN_REQ_ERR_INV_DSTADDR;
        return st;
    }
    else if (s5st == SOCKS5ADDR_DONE)
        st = p->state = CONN_REQ_BND_PORT;
    // Successfully read Address
    while(buffer_can_read(b) && !connection_req_done_parsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = connection_req_read_next_byte(p, c);
    }
    return st;
}
int connection_req_done_parsing(connection_req_parser p, int * errored) {
    return p->state >= CONN_REQ_DONE;
}
// Free all connection_req_parser-Related memory

void free_connection_req_parser(connection_req_parser p) {
    free_socks_5_addr_parser(p->socks_5_addr_parser);
    free(p);
}



connection_req_state get_con_state(connection_req_parser parser){ return parser -> state;}