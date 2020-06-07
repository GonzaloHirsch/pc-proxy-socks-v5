#include "parsers/ConnectionReqParser.h"
#include "parsers/SOCKS5AddrParser.h"

typedef struct ConnectionReq {
    // public:
    uint8_t ver;
    uint8_t cmd;
    uint8_t rsv;
    uint8_t dstPort[2];
} ConnectionReq;

struct ConnectionReqParser {
    // public:
    ConnectionReq finalMessage;
    ConnectionReqState state;
    // private:
    Socks5AddrParser socks5AddrParser;
    unsigned int bytesToRead;
};

typedef enum CMDType {
    TCP_IP_STREAM = 0x01,
    TCP_IP_PORT_BINDING = 0x02,
    UDP = 0x03
} CMDType;


ConnectionReqParser newConnectionReqParser() {
    ConnectionReqParser crp = malloc(sizeof(struct ConnectionReqParser));
    crp->socks5AddrParser = NULL;
    return crp;
}

enum ConnectionReqState connectionReqReadNextByte(ConnectionReqParser p, const uint8_t b) {
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
                p->bytesToRead = 2; // Untidy, this is for BND_PORT
            }
            else 
                p->state = CONN_REQ_ERR_INV_RSV;
            break;
        case CONN_REQ_DSTADDR:
            // this state is just for consume function
            // to know when to use socks5addrParser
            break;
        case CONN_REQ_BND_PORT:
            if (p->bytesToRead) {
                p->finalMessage.dstPort[2-p->bytesToRead] = b; //TODO do sth about magic number
                p->bytesToRead--;
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
enum ConnectionReqState connectionReqConsumeMessage(buffer * b, ConnectionReqParser p, int *errored) {
    ConnectionReqState st = p->state;
    while(buffer_can_read(b) && p->state != CONN_REQ_DSTADDR && !connectionReqDoneParsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = connectionReqReadNextByte(p, c);
    }
    // Reading Address Part...
    p->socks5AddrParser = newSocks5AddrParser();
    Socks5AddrState s5st = socks5AddrConsumeMessage(b, p->socks5AddrParser, errored);
    if (s5st > SOCKS5ADDR_DONE) {
        st = p->state = CONN_REQ_ERR_INV_DSTADDR;
        return st;
    }
    else if (s5st == SOCKS5ADDR_DONE)
        st = p->state = CONN_REQ_BND_PORT;
    // Successfully read Address
    while(buffer_can_read(b) && !connectionReqDoneParsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = connectionReqReadNextByte(p, c);
    }
    return st;
}
int connectionReqDoneParsing(ConnectionReqParser p, int * errored) {
    return p->state >= CONN_REQ_DONE;
}
// Free all ConnectionReqParser-Related memory

void freeConnectionReqParser(ConnectionReqParser p) {
    freeSocks5AddrParser(p->socks5AddrParser);
    free(p);
}