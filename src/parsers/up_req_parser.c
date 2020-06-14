#include "parsers/up_req_parser.h"

void up_req_parser_init(up_req_parser uprp) {
    memset(uprp, 0, sizeof(struct up_req_parser));
    uprp->state = UP_REQ_VERSION;
    uprp->uid = NULL;
    uprp->pw = NULL;
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
                p->uid = malloc(p->uidLen);
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
                p->pw = malloc(p->pwLen);
                p->bytes_to_read = b;
            }
            break;
        case UP_REQ_PW:
            p->pw[p->pwLen - p->bytes_to_read] = b;
            p->bytes_to_read--;
            if (p->bytes_to_read <= 0) {
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

    return p->state;
}

enum up_req_state up_consume_message(buffer * b, up_req_parser p, bool *errored) {
    enum up_req_state st = p->state;

    while (buffer_can_read(b) && !up_done_parsing(p->state, errored)) {
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

int up_done_parsing(up_req_state st, bool * errored) {
    if (st > UP_REQ_DONE )
        *errored = true;
    return st >= UP_REQ_DONE;
}

void free_up_req_parser(up_req_parser p) {
    if(p->uid != NULL)
        free(p->uid);
    if(p->pw != NULL)
        free(p->pw);
}


extern int
up_marshall(buffer *b, const uint8_t status) {
    size_t n;
    uint8_t *buff = buffer_write_ptr(b, &n);
    if(n < 2) {
        return -1;
    }
    buff[0] = 0x05;
    buff[1] = status;
    buffer_write_adv(b, 2);
    return 2;
}


up_req_state get_up_req_state(up_req_parser parser){ return parser -> state; }