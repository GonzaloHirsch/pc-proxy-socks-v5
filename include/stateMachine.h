#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__

#include <stdlib.h>
#include <stdio.h>
#include "../include/selector.h"
#include <stdint.h>


/**
 * State Machine for the Socks5 general states
 * 
 * 
 * 
 * 
 * 
 * 
*/

/** General State Machine for a server connection */
typedef enum PossibleStates
{
    /**
     * State when receiving the client initial "hello" and processing
     * 
     * Interests:
     *  - OP_READ -> Reading over the client fd
     * 
     * Transitions:
     *  - HELLO_READ -> While the message is not complete
     *  - HELLO_WRITE -> When the message is complete
     *  - ERROR -> In case of an error
     * */
    HELLO_READ,

    /**
     * State when sending the client initial "hello" response
     * 
     * Interests:
     *  - OP_WRITE -> Writing over the client fd
     * 
     * Transitions:
     *  - HELLO_WRITE -> While there are bytes to send
     *  - REQUEST_READ -> When all the bytes have been transfered
     *  - ERROR -> In case of an error
     * */
    HELLO_WRITE,

    /**
     * State when receiving the client request
     * 
     * Interests:
     *  - OP_READ -> Reading over the client fd
     * 
     * Transitions:
     *  - REQUEST_READ -> While the message is not complete
     *  - RESOLVE -> When the message is complete
     *  - ERROR -> In case of an error
     * */
    REQUEST_READ,

    /**
     * State when the client uses a FQDN to connect to another server, the name must be resolved
     * 
     * Interests:
     *  - OP_READ -> Reading over the resolution response fd
     *  - OP_WRITE -> Writing to the DNS server the request
     * 
     * Transitions:
     *  - RESOLVE -> While the name is not resolved
     *  - CONNECTING -> When the name is resolved and the IP address obtained
     *  - ERROR -> In case of an error
     * */
    RESOLVE,

    /**
     * State when connecting to the server the client request is headed to
     * 
     * Interests:
     * 
     * Transitions:
     *  - CONNECTING -> While the server is establishing the connection with the destination of the request
     *  - REPLY -> When the connection is successful and our server can send the request
     *  - ERROR -> In case of an error
     * */
    CONNECTING,

    /**
     * State when sending/receiving the request to/from the destination server
     * 
     * Interests:
     *  - OP_READ -> Reading over the server response
     *  - OP_WRITE -> Writing to server the request
     * 
     * Transitions:
     *  - REPLY -> While the request is being sent and the response is still being copied
     *  - COPY -> When the server response is finished
     *  - ERROR -> In case of an error
     * */
    REPLY,

    /**
     * State when copying the response to the client fd
     * 
     * Interests:
     *  - OP_WRITE -> Writing to client fd
     * 
     * Transitions:
     *  - COPY -> While the server is still copying the response
     *  - DONE -> When the copy process is finished
     *  - ERROR -> In case of an error
     * */
    COPY,

    /** Terminal state */
    DONE,
    /** Terminal state */
    ERROR,
} PossibleStates;

struct state {
    // State type of the state 
    PossibleStates state;
    // Next state to move 
    state next_state;

    /*
    // Executed on state enter 
    void (*on_enter) (state_machine sm, struct selector_key sk);
    // Executed on state exit 
    void (*on_exit) (state_machine sm, struct selector_key sk);
    // Executed on error 
    void (*on_error) (state_machine sm, struct selector_key sk);
    // Executed on available information to read 
    void (*on_available_read) (state_machine sm, struct selector_key sk);
    // Executed on available information to write 
    void (*on_available_write) (state_machine sm, struct selector_key sk);
    */
};
typedef struct state * state;

struct state_machine{
    /** Current state the machine is on */
    state current_state;
    /** Array with all the available states */
    state all_states;
    /** Count of all available states */
    uint8_t state_count;
} ;
typedef struct state_machine * state_machine;


void init_state_machine(state_machine sm);

void destroy_state_machine(state_machine sm);

state get_current_state(state_machine sm);

state handle_on_state_exit(state_machine sm);

state handle_on_state_enter(state_machine sm);

state handle_on_error(state_machine sm);

state handle_on_available_read(state_machine sm);

state handle_on_available_write(state_machine sm);

#endif