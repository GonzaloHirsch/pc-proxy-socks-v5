#include "parsers/up_req_parser.h"

struct up_req_parser {
    // public:      //
    void * data;
    char * uid;
    char * pw;
    // private:     //
    up_req_state state;
    uint8_t uidLen;
    uint8_t pwLen;
    uint8_t bytes_to_read;
};

up_req_parser new_up_req_parser() {
    up_req_parser uprp = malloc(sizeof(struct up_req_parser));
    uprp->state = UP_REQ_VERSION;
    return uprp;
}

enum up_req_state up_read_next_byte(up_req_parser p, const uint8_t b) {   
    switch(p->state) {
        case UP_REQ_VERSION:
            if (0x01 == b)
                p->state = UP_REQ_IDLEN;
            else
                p->state = UP_ERROR_INV_VERSION;
            break;
        case UP_REQ_IDLEN:
            if (b == 0)
                p->state = UP_ERROR_INV_IDLEN;
            else { // (b <= 255)
                p->state = UP_REQ_ID;
                p->uidLen = b;
                // TODO RFC doesn't specify null-terminator. Add one either way
                p->uid = malloc(p->uidLen + 1);
                p->bytes_to_read = b;
            }
            break;
        case UP_REQ_ID:
            // if (p->bytes_to_read > 0) {
            p->uid[p->uidLen - p->bytes_to_read] = b;
            p->bytes_to_read--;
            if (p->bytes_to_read <= 0) {
                p->uid[p->uidLen+1] = '\0';
                p->state = UP_REQ_PWLEN;            
            }
            break;
        case UP_REQ_PWLEN:
            if (b == 0)
                p->state = UP_ERROR_INV_PWLEN;
            else { // (b <= 255)
                p->state = UP_REQ_PW;
                p->pwLen = b;
                p->pw = malloc(p->pwLen + 1);
                p->bytes_to_read = b;
            }
            break;
        case UP_REQ_PW:
            p->pw[p->pwLen - p->bytes_to_read] = b;
            p->bytes_to_read--;
            if (p->bytes_to_read <= 0) {
                p->pw[p->pwLen+1] = '\0';
                p->state = UP_REQ_DONE;            
            }
            break;
        case UP_REQ_DONE:

            break;
        case UP_ERROR_INV_VERSION:
            // break;
        case UP_ERROR_INV_IDLEN:
            // break;
        case UP_ERROR_INV_PWLEN:
            // break;
        case UP_ERROR_INV_AUTH:
            // break;
        default:
            break;
    }
}

enum up_req_state up_consume_message(buffer * b, up_req_parser p, int *errored) {
    enum up_req_state st = p->state;

    while (buffer_can_read(b) && !up_done_parsing(p, errored)) {
        const uint8_t c = buffer_read(b);
        st = up_read_next_byte(p, c);
    }
    return st;
}

const char * upErrorString(const up_req_parser p) {
    switch (p->state) {
        case UP_ERROR_INV_VERSION:
            return "Invalid (Unsupported) Version"; 
        case UP_ERROR_INV_IDLEN:
            return "Invalid ID length"; 
        case UP_ERROR_INV_PWLEN:
            return "Invalid Password Length"; 
        case UP_ERROR_INV_AUTH:
            return "Invalid Auth Data";
        }
}

int up_done_parsing(up_req_parser p, int * errored) {
    switch(p->state) {
        case UP_REQ_DONE:
            *errored = 0; 
            return 1;
        case UP_ERROR_INV_VERSION:
            *errored = UP_ERROR_INV_VERSION;
        case UP_ERROR_INV_IDLEN:
            *errored = UP_ERROR_INV_IDLEN;
        case UP_ERROR_INV_PWLEN:
            *errored = UP_ERROR_INV_PWLEN;    
        case UP_ERROR_INV_AUTH:
            *errored = UP_ERROR_INV_AUTH;
            return 1;
        default:
            *errored = 0;
            return 0;
    }
}

void free_up_req_parser(up_req_parser p) {
    free(p->data);
    free(p->uid);
    free(p->pw);
    free(p);
}