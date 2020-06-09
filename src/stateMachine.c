#include "stateMachine.h"

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