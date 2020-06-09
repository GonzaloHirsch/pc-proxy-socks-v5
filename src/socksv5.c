
#include "../include/socksv5.h"
#include "../Utility.h"

// -------------- INTERNAL FUNCTIONS-----------------------------------
void masterSocketHandle(struct selector_key *key);
void masterSocketHandleClose(struct selector_key *key);
void slaveSocketHandleRead(struct selector_key *key);
void slaveSocketHandleWrite(struct selector_key *key);
void slaveSocketHandleBlock(struct selector_key *key);
void slaveSocketHandleClose(struct selector_key *key);

void initSocksState(Socks5 *sockState);
void freeSocksState(Socks5 *sockState);

void renderToState(struct selector_key *key, char *received, int valread);

static void signal_handler(const int signal);

// Address for socket binding
struct sockaddr_in *address;

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
    int opt = TRUE;
    int master_socket;

    // Selector for concurrent connexions
    fd_selector selector = NULL;

    // Setting the socket binding for the server to work ok
    address = malloc(sizeof(address));
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);

    // ----------------- INITIALIZE THE MAIN SOCKET -----------------

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
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding the socket to localhost:1080
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        //log(FATAL, "bind failed");
        exit(EXIT_FAILURE);
    }

    // Checking if the socket is able to listen
    if (listen(master_socket, MAX_PENDING_CONNECTIONS) < 0)
    {
        //log(FATAL, "listen");
        exit(EXIT_FAILURE);
    }

    // ----------------- INITIALIZING THE SCTP SOCKET -----------------

    // TODO: CREATE SCTP SOCKET HERE!

    // ----------------- CREATING SIGNAL HANDLERS -----------------

    // Handling the SIGTERM signal, in order to make server stopping more clean and be able to free resources
    signal(SIGTERM, signal_handler);

    // ----------------- CREATE THE SELECTOR -----------------

    // Creating the configuration for the select
    const struct selector_init selector_configuration = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10, // Time in seconds
            .tv_nsec = 0  // Time in nanos
        }};

    // Initializing the selector using the created configuration
    if (selector_init(&selector_configuration) == 0)
    {
        perror("Initializing the selector");
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
    masterSocketHandler->handle_read = masterSocketHandle;
    masterSocketHandler->handle_write = NULL;
    masterSocketHandler->handle_block = NULL;
    masterSocketHandler->handle_close = masterSocketHandleClose;

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

/*  Handle for the master socket.
    Used to handle new connections.
*/
void masterSocketHandle(struct selector_key *key)
{

    int newSocket;
    socklen_t addrlen = (socklen_t)sizeof(*address);

    // Accpet the new socket.
    if (newSocket = accept(newSocket, (struct sockaddr *)address, &addrlen) < 0)
    {
        perror("Error: Error accepting new socket\n");
        exit(EXIT_FAILURE);
    }

    // Inform user of socket number - used in send and receive commands
    printf("New connection , socket fd is %d , ip is : %s , port : %d \n", newSocket, inet_ntoa(address->sin_addr), ntohs(address->sin_port));

    // Handler for the slave sockets.
    // Create socket handler for the master socket
    fd_handler *slaveSocketHandler = malloc(sizeof(fd_handler));
    slaveSocketHandler->handle_read = slaveSocketHandleRead;
    slaveSocketHandler->handle_write = slaveSocketHandleWrite;
    slaveSocketHandler->handle_block = slaveSocketHandleBlock;
    slaveSocketHandler->handle_close = masterSocketHandleClose;

    // Initialize the Socks5 structure which contain the state machine and other info for the socket.
    Socks5 *sockState = malloc(sizeof(Socks5));
    initSocksState(sockState);

    // Register the new file descriptor.
    selector_status resultStatus;
    resultStatus = selector_register(key->s, newSocket, slaveSocketHandler, OP_READ + OP_WRITE, (void *)sockState);
    if (resultStatus != SELECTOR_SUCCESS)
    {
        printf("Error: %s while creating registering new fd", selector_error(resultStatus));
    }
}

// TODO: implement
void masterSocketHandleClose(struct selector_key *key)
{
}

void slaveSocketHandleRead(struct selector_key *key)
{

    int sd = key->fd, valread;
    char received[BUFFERSIZE + 1];
    socklen_t addrlen = (socklen_t)sizeof(*address);

    //Check if it was for closing , and also read the incoming message
    if ((valread = read(sd, received, BUFFERSIZE)) <= 0)
    {
        // Remove from selector.
        // This will call slaveSocketHandleClose --> All cleaning ops go there.
        selector_status responseStatus = selector_unregister_fd(key->s, key->fd);
        if (responseStatus != SELECTOR_SUCCESS)
        {
            printf("Error: %s while processing read\n", selector_error(responseStatus));
        }
    }
    else
    {
        printf("Received %d bytes from socket %d\n", valread, sd);

        //TODO: To implement shortly
        //render_to_state();
    }
}

/*  Handle for write of slave socket.
*/
void slaveSocketHandleWrite(struct selector_key *key)
{

    Socks5 *sockState = (Socks5 *)key->data;
    buffer *writeBuffer = (buffer *)sockState->writeBuffer;
    int sd = key->fd;

    size_t bytesToSend = writeBuffer->read - writeBuffer->write;
    if (bytesToSend > 0)
    { // Puede estar listo para enviar, pero no tenemos nada para enviar
        printf("Trying to send %zu bytes to socket %d\n", bytesToSend, sd);
        size_t bytesSent = send(sd, writeBuffer->read, bytesToSend, MSG_DONTWAIT); // | MSG_NOSIGNAL
        printf("Sent %zu bytes\n", bytesSent);

        if (bytesSent < 0)
        {
            // Esto no deberia pasar ya que el socket estaba listo para escritura
            // TODO: manejar el error
            perror(" ");
        }
        else
        {
            size_t bytesLeft = bytesSent - bytesToSend;

            // Si se pudieron mandar todos los bytes limpiamos el buffer
            // y sacamos el fd para el select
            if (bytesLeft == 0)
            {
                buffer_reset(writeBuffer);
            }
            else
            {
                writeBuffer->read += bytesSent;
                buffer_compact(writeBuffer);
            }
        }
    }
}

// TODO: Implement
void slaveSocketHandleBlock(struct selector_key *key)
{
}

//TODO: Implement
void slaveSocketHandleClose(struct selector_key *key)
{
}

void initSocksState(Socks5 *sockState)
{
    if (sockState == NULL)
    {
        perror("Error: Initizalizing null Socks5\n");
    }

    stm_init(&(sockState->stm));

    // Write Buffer for the socket(Initialized)
    buffer *writeBuffer = malloc(sizeof(buffer));
    buffer_init(writeBuffer, BUFFERSIZE + 1, malloc(BUFFERSIZE + 1));
    sockState->writeBuffer = writeBuffer;
}

// Todo: Implement
void freeSocksState(Socks5 *sockState)
{
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