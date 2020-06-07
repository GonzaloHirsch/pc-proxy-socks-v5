#include "parsers/SOCKS5AddrParser.h"

struct Socks5AddrParser {
    // public:
    uint8_t type; // TODO uint8 + packed? Worth it?
    uint8_t * addr;
    // private:
    uint8_t addrLen;
    Socks5AddrState state;
    int bytesToRead;
};

typedef enum S5AddrType {
    IP_V4_T = 0x01,
    DNAME_T = 0x03,
    IP_V6_T = 0x04
} S5AddrType;

typedef enum AddressSize {
    IP_V4_ADDR_SIZE = 4,
    IP_V6_ADDR_SIZE = 16
} AddressSize;

Socks5AddrParser newSocks5AddrParser() {
    Socks5AddrParser s5ap = malloc(sizeof(struct Socks5AddrParser));
    s5ap->type = 0;
    s5ap->bytesToRead = 0;
    s5ap->addrLen = 0;
    s5ap->state = SOCKS5ADDR_TYPE;
    return s5ap;
}

enum Socks5AddrState socks5AddrReadNextByte(Socks5AddrParser p, const uint8_t b) {
    switch(p->state) {
        case SOCKS5ADDR_TYPE:
            if (b == IP_V4_T) {
                p->bytesToRead = IP_V4_ADDR_SIZE;
                p->addrLen = IP_V4_ADDR_SIZE;
                p->addr = malloc(IP_V4_ADDR_SIZE);
                p->state = SOCKS5ADDR_IP_V4;
            }
            else if (b == DNAME_T) {
                p->bytesToRead = 1;
                p->state = SOCKS5ADDR_DNAME_LEN;
            }
            else if (b == IP_V6_T) {
                p->bytesToRead = IP_V6_ADDR_SIZE;
                p->addrLen = IP_V6_ADDR_SIZE;
                p->addr = malloc(IP_V6_ADDR_SIZE);
                p->state = SOCKS5ADDR_IP_V6;
            }
            else
                p->state = SOCKS5ADDR_ERR_INV_TYPE;
            p->type = b;
            break;
        case SOCKS5ADDR_DNAME_LEN:
            p->bytesToRead = b;
            p->addrLen = b;
            p->addr = malloc(b+1);
            p->state = SOCKS5ADDR_DNAME;
            break;
        case SOCKS5ADDR_DNAME:
        case SOCKS5ADDR_IP_V4:
        case SOCKS5ADDR_IP_V6:
            if (p->bytesToRead) {
                p->addr[p->addrLen - p->bytesToRead] = b;
                p->bytesToRead--;
                if (p->bytesToRead == 0)
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
enum Socks5AddrState socks5AddrConsumeMessage(buffer * b, Socks5AddrParser p, int *errored) {
    Socks5AddrState st = p->state;
    while(buffer_can_read(b) && !socks5AddrDoneParsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = socks5AddrReadNextByte(p, c);
    }
    if (!buffer_can_read(b) && !socks5AddrDoneParsing(p, errored)) {
        p->state = st = SOCKS5ADDR_ERR_INV_ADDRESS;
    }
    return st;
}
int socks5AddrDoneParsing(Socks5AddrParser p, int * errored) {
    return p->state >= SOCKS5ADDR_DONE;
}

const char * getSocks5Address(Socks5AddrParser p) {
    return p->addr;
}
const int getSocks5Type(Socks5AddrParser p) {
    return p->type;
}

// Free all Socks5AddrParser-Related memory

void freeSocks5AddrParser(Socks5AddrParser p) {
    free(p->addr);
    free(p);
}