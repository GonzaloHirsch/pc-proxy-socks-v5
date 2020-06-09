#include "parsers/hello_parser.h"

hello_parser new_hello_parser() {
    hello_parser hp = calloc(1, sizeof(struct hello_parser));
    return hp;
}

enum hello_state hello_read_next_byte(hello_parser p, const uint8_t b) {
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
            p->bytes_to_read = b;
            p->state = HELLO_AUTH;
            break;
        case HELLO_AUTH:
            if (p->bytes_to_read) {
                p->auth[p->nauth - p->bytes_to_read] = b;
                p->bytes_to_read--;
                if (p->bytes_to_read == 0)
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

enum hello_state hello_consume_message(buffer * b, hello_parser p, int *errored) {
    hello_state st = p->state;
    while(buffer_can_read(b) && !hello_done_parsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = hello_read_next_byte(p, c);
    }
    return st;
}

int hello_done_parsing(hello_parser p, int * errored) {
    return p->state >= HELLO_DONE;
}

uint8_t get_n_auth(hello_parser p) {
    return p->nauth;
}
const uint8_t * get_auth_types(hello_parser p) {
    return p->auth;
}

// Free all hello_parser-Related memory
void free_hello_parser(hello_parser p) {
    free(p->auth);
    free(p);
}


hello_state get_hello_state(hello_parser parser){ return parser -> state;}