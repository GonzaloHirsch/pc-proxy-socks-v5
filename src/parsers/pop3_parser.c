#include "parsers/pop3_parser.h"

#define TRUE 1
#define FALSE 0
#define NL '\n'

void pop3_parser_init(pop3_parser p){
    memset(p, 0, sizeof(struct pop3_parser));
    p -> state = POP3_NL;
}


pop3_state pop3_consume_byte(pop3_parser p, uint8_t b){
    switch (p -> state)
    {
    case POP3_NL:
        if(b == 'u' || b == 'U'){
            p -> state = POP3_U_U;
        }
        else if(b == 'p' || b == 'P'){
            p -> state = POP3_P_P;
        }
        break;
    
    case POP3_READING:
        if(b == NL){
            p ->state = POP3_NL;
        }

        break;
    case POP3_U_U:
        if(b == 's' || b == 'S'){
            p -> state = POP3_U_S;
        }
        else
        {
            p -> state = POP3_READING;
        }

        break;

    case POP3_U_S:
        if(b == 'e' || b == 'E'){
            p -> state = POP3_U_E;
        }
        else
        {
            p -> state = POP3_READING;
        }

    break;

    case POP3_U_E:
        if(b == 'r' || b == 'R'){
            p -> state = POP3_U_S;
        }
        else
        {
            p -> state = POP3_READING;
        }

        break;
    case POP3_U_R:
        if(b == ' '){
            p -> state = POP3_G_USER;
            p ->cursor = p -> buff;
            p -> start = p -> buff;
        }
        else
        {
            p -> state = POP3_READING;
        }

        break;

    case POP3_G_USER:
        if(b != NL){
            *p -> cursor = b;
            p ->cursor ++; 
        }
        else
        {
            *p ->cursor = '\0';
            p -> user = malloc(strlen((const char *) p -> start) + 1);
            strcpy((char *)p -> user,(char *) p ->start);
            p ->h_user = 1; //have to check if user is already submitted to store password
            p -> state = POP3_NL;
        }
        break;

    case POP3_P_P:
        if(b == 'a' || b == 'A'){
            p -> state = POP3_P_A;
        }
        else{
            p -> state = POP3_READING;
        }
        break;

    case POP3_P_A:
        if(b == 's' || b == 'S'){
            p -> state = POP3_P_S1;
        }
        else{
            p -> state = POP3_READING;
        }
        break;

    case POP3_P_S1:
        if(b == 's' || b == 'S'){

            p -> state = POP3_P_S2;

        }
        else{
            p -> state = POP3_READING;
        }

        break;
    case POP3_P_S2:
        if(b == ' ' || b == ' '){
            if(p -> h_user){
                p -> state = POP3_G_PASS;
                p ->cursor = p -> buff;
                p -> start = p -> buff;
            }
            else
            {
                p -> state = POP3_ERR_NO_U; //if no user has been given it has to be an error
            }
            

        }
        else{
            p -> state = POP3_READING;
        }

        break;

    case POP3_G_PASS:
        if(b != NL){
            *p ->cursor = b;
            p -> cursor ++;
        }
        else
        {
             *p ->cursor = '\0';
             p -> pass = malloc(strlen((const char *) p ->start) + 1);
             strcpy((char *)p ->pass,(char *) p ->start);
             p -> state = POP3_F;
        }

    case POP3_F:

        break;

    case POP3_ERR_NO_U:

        break;
        
    
        
    default:
        break;
    }

    return p -> state;
}

int pop3_done_parsing(pop3_parser p,int * errored){
    if(p -> state >= POP3_F || *errored == 1){
        return TRUE;
    }

    return FALSE;
}

pop3_state pop3_consume_msg(buffer * b, pop3_parser p, int * errored){
    pop3_state st = p -> state;
    while(buffer_can_read(b) && !pop3_done_parsing(p,errored)){
        uint8_t c = buffer_read(b);
        st = pop3_consume_byte(p, c);
    }

    return st;

}


void free_pop3_parser(pop3_parser p){

    if(p -> user != NULL){
        free(p ->user);
    }
    if(p -> pass != NULL){
        free(p -> pass);
    }
}
