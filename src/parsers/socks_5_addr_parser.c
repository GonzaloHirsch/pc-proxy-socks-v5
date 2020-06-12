#include "parsers/socks_5_addr_parser.h"

typedef enum S5AddrType {
    IP_V4_T = 0x01,
    DNAME_T = 0x03,
    IP_V6_T = 0x04
} S5AddrType;

typedef enum AddressSize {
    IP_V4_ADDR_SIZE = 4,
    IP_V6_ADDR_SIZE = 16
} AddressSize;

void socks_5_addr_parser_init(socks_5_addr_parser s5ap) {
    memset(s5ap, 0, sizeof(struct socks_5_addr_parser));
    s5ap->state = SOCKS5ADDR_TYPE;
}

enum socks_5_addr_state socks_5_addr_read_next_byte(socks_5_addr_parser p, const uint8_t b) {
    switch(p->state) {
        case SOCKS5ADDR_TYPE:
            if (b == IP_V4_T) {
                p->bytes_to_read = IP_V4_ADDR_SIZE;
                p->addrLen = IP_V4_ADDR_SIZE;
                p->addr = malloc(IP_V4_ADDR_SIZE);
                p->state = SOCKS5ADDR_IP_V4;
            }
            else if (b == DNAME_T) {
                p->bytes_to_read = 1;
                p->state = SOCKS5ADDR_DNAME_LEN;
            }
            else if (b == IP_V6_T) {
                p->bytes_to_read = IP_V6_ADDR_SIZE;
                p->addrLen = IP_V6_ADDR_SIZE;
                p->addr = malloc(IP_V6_ADDR_SIZE);
                p->state = SOCKS5ADDR_IP_V6;
            }
            else
                p->state = SOCKS5ADDR_ERR_INV_TYPE;
            p->type = b;
            break;
        case SOCKS5ADDR_DNAME_LEN:
            p->bytes_to_read = b;
            p->addrLen = b;
            p->addr = malloc(b+1);
            p->state = SOCKS5ADDR_DNAME;
            break;
        case SOCKS5ADDR_DNAME:
        case SOCKS5ADDR_IP_V4:
        case SOCKS5ADDR_IP_V6:
            if (p->bytes_to_read) {
                p->addr[p->addrLen - p->bytes_to_read] = b;
                p->bytes_to_read--;
                if (p->bytes_to_read == 0)
                    p->state = SOCKS5ADDR_DONE;
            }
            else {
                if (p->state == SOCKS5ADDR_DNAME)
                    p->addr[p->addrLen] = '\0';
                p->state = SOCKS5ADDR_DONE;            
            }
            break;
        case SOCKS5ADDR_DONE:
            
            break;
        case SOCKS5ADDR_ERR_INV_TYPE:
            
            break;
        case SOCKS5ADDR_ERR_INV_ADDRESS:
            break;
    }
    return p->state;
}
enum socks_5_addr_state socks_5_addr_consume_message(buffer * b, socks_5_addr_parser p, bool *errored) {
    socks_5_addr_state st = p->state;
    while(buffer_can_read(b) && !socks_5_addr_done_parsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = socks_5_addr_read_next_byte(p, c);
    }
    if (!buffer_can_read(b) && !socks_5_addr_done_parsing(p, errored)) {
        p->state = st = SOCKS5ADDR_ERR_INV_ADDRESS;
    }
    return st;
}
int socks_5_addr_done_parsing(socks_5_addr_parser p, bool * errored) {
    return p->state >= SOCKS5ADDR_DONE;
}

const char * get_socks_5_address(socks_5_addr_parser p) {
    return p->addr;
}
const int get_socks_5_type(socks_5_addr_parser p) {
    return p->type;
}

// Free all socks_5_addr_parser-Related memory

void free_socks_5_addr_parser(socks_5_addr_parser p) {
    free(p->addr);
    free(p);
}



socks_5_addr_state get_socks5_state(socks_5_addr_parser parser){return parser -> state;}