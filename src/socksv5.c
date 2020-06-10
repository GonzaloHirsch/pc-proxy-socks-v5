
#include "../include/socksv5.h"

// -------------- INTERNAL FUNCTIONS-----------------------------------

void renderToState(struct selector_key *key, char *received, int valread);

static void signal_handler(const int signal);

// Static boolean to be used as a way to end the server with a signal in a more clean way
static bool finished = false;

/**
 * Handler for an interruption signal, just changes the server state to finished
 * @param signal integer representing the signal sent
 * 
 * */
static void signal_handler(const int signal)
{
    printf("Received signal %d, exiting now", signal);
    finished = true;
}

int main()
{
    printf("Starting server.\n");

    int opt = TRUE;
    int master_socket;

    // Selector for concurrent connexions
    fd_selector selector = NULL;

    // Address for socket binding
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // ----------------- INITIALIZE THE MAIN SOCKET -----------------

    printf("Initializing main socket\n");

    // Creating the server socket to listen
    master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket <= 0)
    {
        printf("socket failed");
        //log(FATAL, "socket failed");
        exit(EXIT_FAILURE);
    }

    // Setting the master socket to allow multiple connections, not required, just good habit
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("ERROR: Failure setting setting master socket\n");
        exit(EXIT_FAILURE);
    }

    // Binding the socket to localhost:1080
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("ERROR: Failure binding master socket\n");
        exit(EXIT_FAILURE);
    }

    // Checking if the socket is able to listen
    if (listen(master_socket, MAX_PENDING_CONNECTIONS) < 0)
    {
        perror("ERROR: Failure listening master socket\n");
        exit(EXIT_FAILURE);
    }

    // ----------------- INITIALIZING THE SCTP SOCKET -----------------

    // TODO: CREATE SCTP SOCKET HERE!

    // ----------------- CREATING SIGNAL HANDLERS -----------------

    // Handling the SIGTERM signal, in order to make server stopping more clean and be able to free resources
    signal(SIGTERM, signal_handler);

    // ----------------- CREATE THE SELECTOR -----------------

    printf("Creating selector\n");

    // Creating the configuration for the select
    const struct selector_init selector_configuration = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10, // Time in seconds
            .tv_nsec = 0  // Time in nanos
        }};

    // Initializing the selector using the created configuration
    if (selector_init(&selector_configuration) != SELECTOR_SUCCESS)
    {
        perror("Error: Initializing the selector");
        exit(EXIT_FAILURE);
    }

    // Instancing the selector
    selector = selector_new(SELECTOR_MAX_ELEMENTS);
    if (selector == NULL)
    {
        perror("Creating the selector");
        exit(EXIT_FAILURE);
    }

    // Create socket handler for the master socket
    fd_handler *masterSocketHandler = malloc(sizeof(fd_handler));
    masterSocketHandler->handle_read = socksv5_passive_accept;
    masterSocketHandler->handle_write = NULL;
    masterSocketHandler->handle_block = NULL;
    //TODO: define on close
    masterSocketHandler->handle_close = NULL;

    // TODO: Create socket handler for the SCTP socket

    // Register the master socket to the managed fds
    selector_status ss_master = SELECTOR_SUCCESS;
    ss_master = selector_register(selector, master_socket, masterSocketHandler, OP_READ, NULL);
    if (ss_master != SELECTOR_SUCCESS)
    {
        printf("Error in master socket: %s", selector_error(ss_master));
    }

    // TODO: Register the SCTP socket to the managed fds

    // Accept the incoming connection
    printf("Waiting for connections on socket %d\n", master_socket);

    // Cycle until a signal is received as finished
    while (!finished)
    {
        printf("Awaiting connection\n");
        
        // Wait for activiy of one of the sockets:
        // -Master Socket --> New connection.
        // -Child Sockets --> Read or write operation
        ss_master = selector_select(selector);
        if (ss_master != SELECTOR_SUCCESS)
        {
            printf("Error awaiting for select: %s\n", selector_error(ss_master));
        }
    }

    // TODO: FREE ALL RESOURCES

    return 0;
}






/*
void renderToState(struct selector_key * key, char * received, int valread){
    
    int errored = 0;
    int sd = key->fd;
    Socks5 * sockState = (Socks5 *)key->data;

    //TODO: change whats in this switch based on the state structure.
    switch (sockState->stm->current_state->state){

        case HELLO_READ:
        
            hello_state hs = hello_consume_message(received, sockState->client.hello.parser, &errored);

            if (errored){
                perror("Error during hello parsing");
                exit(EXIT_FAILURE);
            }
            if (hs == DONE){
                sockState->stm = HELLO_WRITE;
                free_hello_parser(sockState->client.hello.parser);
            }

            break;
        

        case HELLO_WRITE:

            break;

        case REQUEST_READ:

            break;

        case RESOLVE:

            break;

        case CONNECTING:

            break;

        case REPLY:

            break;

        case COPY:

            break;

        case DONE:

            break;

        case ERROR:

            break;

        default:
            break;
    }


}



void write_to_client(struct selector_key * key){
    int sd = key -> fd;
    int errored = 0;
    Socks5 * sock_state = (Socks5 *) key -> data;
    int state = get_current_state(sock_state -> stm);

    switch (state)
    {
    case HELLO_WRITE:
        char response[] = {0x05, 0x00}; //temporary change to 0x02 when auth is done

        write(sd, response, N(response));

        set_current_state(sock_state, REQUEST_READ);

        break;

    case RESOLVE:

        break;
    
    case CONNECTING:

        break;

    case REPLY:
        char rep;
        rep = 0x00; //just for now it succeeds every request

        //need to check how to retrieve port and address

        break;
    

    
    default:
        break;
    }

}

*/