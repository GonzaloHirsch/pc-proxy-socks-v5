#include "stateMachine.h"

typedef struct state_machine
{
    /** Current state the machine is on */
    state current_state;
    /** Array with all the available states */
    state all_states;
    /** Count of all available states */
    uint8_t state_count;
};

typedef struct state {
    /** State type of the state */
    PossibleStates state;
    /** Executed on state enter */
    void (*on_enter)
    /** Executed on state exit */
    void (*on_exit) 
    /** Executed on error */
    void (*on_error)
    /** Executed on available information to read */
    void (*on_available_read)
    /** Executed on available information to write */
    void (*on_available_write)
};

void init_state_machine(state_machine sm){
    // TODO: CREATE ALL STATES AND INIT MACHINE
}

void destroy_state_machine(state_machine sm){
    // TODO: FREE AL RESOURCES
}

state get_current_state(state_machine sm){
    if (sm != NULL){
        return sm->current_state;
    }
    return NULL;
}

state handle_on_state_exit(state_machine sm);

state handle_on_state_enter(state_machine sm);

state handle_on_error(state_machine sm);

state handle_on_available_read(state_machine sm);

state handle_on_available_write(state_machine sm);