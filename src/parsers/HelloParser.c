#include "parsers/HelloParser.h"

struct HelloParser {
    // public:
    uint8_t nauth;
    uint8_t * auth;
    // private:
    unsigned int bytesToRead;
    HelloState state;
};

HelloParser newHelloParser() {
    HelloParser hp = calloc(1, sizeof(struct HelloParser));
    return hp;
}

enum HelloState helloReadNextByte(HelloParser p, const uint8_t b) {
    switch(p->state) {
        case HELLO_VER:
            if (b == 0x05) // TODO do sth about magic number (?) 
                p->state = HELLO_NAUTH;
            else
                p->state = HELLO_ERR_INV_VERSION;            
            break;
        case HELLO_NAUTH:
            // No rules established to know which 
            // cases should be considered as errors
            if (b == 0)
                p->state = HELLO_ERR_INV_NAUTH;
            p->nauth = b;
            p->auth = malloc(b);
            p->bytesToRead = b;
            p->state = HELLO_AUTH;
            break;
        case HELLO_AUTH:
            if (p->bytesToRead) {
                p->auth[p->nauth - p->bytesToRead] = b;
                p->bytesToRead--;
                if (p->bytesToRead == 0)
                    p->state = HELLO_DONE;
            }
            else 
                p->state = HELLO_DONE;
            break;
        case HELLO_DONE:
            
            break;
        case HELLO_ERR_INV_VERSION:
            
            break;
        case HELLO_ERR_INV_NAUTH:
            
            break;
        case HELLO_ERR_UNASSIGNED_METHOD:
            
            break;
        case HELLO_ERR_UNSUPPORTED_METHOD:
            
            break;
    }
    return p->state;
}

enum HelloState helloConsumeMessage(buffer * b, HelloParser p, int *errored) {
    HelloState st = p->state;
    while(buffer_can_read(b) && !helloDoneParsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = helloReadNextByte(p, c);
    }
    return st;
}

int helloDoneParsing(HelloParser p, int * errored) {
    return p->state >= HELLO_DONE;
}

uint8_t getNAuth(HelloParser p) {
    return p->nauth;
}
const uint8_t * getAuthTypes(HelloParser p) {
    return p->auth;
}

// Free all HelloParser-Related memory
void freeHelloParser(HelloParser p) {
    free(p->auth);
    free(p);
}