
#include "../include/socksv5.h"
#include "../Utility.h"

Socks5 *socks_state[MAX_SOCKETS];

// -------------- INTERNAL FUNCTIONS-----------------------------------
void masterSocketHandle(struct selector_key *key);
void masterSocketHandleClose(struct selector_key *key);
void slaveSocketHandleRead(struct selector_key *key);
void slaveSocketHandleWrite(struct selector_key *key);
void slaveSocketHandleBlock(struct selector_key *key);
void slaveSocketHandleClose(struct selector_key *key);


int main()
{
    int opt = TRUE;
    int master_socket, addrlen, new_socket, client_socket[MAX_SOCKETS], max_clients = MAX_SOCKETS, activity, i, sd;
    long val_read;
    int max_sd;
    long valread;

    // Selector for concurrent connexions
    fd_selector selector = NULL; 

    buffer buff;

    char received[BUFFERSIZE + 1];

    char data[BUFFERSIZE + 1]; //data buffer of 2K

    buffer_init(&buff, BUFFERSIZE - 1, data);

    fd_set readfds;
    fd_set writefds;

    struct buffer bufferWrite[MAX_SOCKETS];
    memset(bufferWrite, 0, sizeof bufferWrite);

    for (i = 0; i < max_clients; i++)
    {
        client_socket[i] = 0;
    }

    // Creating the socket binding for the server to work ok
    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // ----------------- INITIALIZE THE MAIN SOCKET -----------------

    // Creating the server socket to listen
    const int master_socket = socket(AF_INET, SOCK_STREAM, 0);
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

    // ----------------- CREATE THE SELECTOR -----------------

    // Creating the configuration for the select
    const struct selector_init selector_configuration = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10, // Time in seconds
            .tv_nsec = 0  // Time in nanos
        }
    };

    // Initializing the selector using the created configuration
    if (selector_init(&selector_configuration) == 0){
        perror("Initializing the selector");
        exit(EXIT_FAILURE);
    }

    // Instancing the selector
    selector = selector_new(SELECTOR_MAX_ELEMENTS);
    if (selector == NULL){
        perror("Creating the selector");
        exit(EXIT_FAILURE);
    }

    // Create socket handler for the master socket
    fd_handler * masterSocketHandler = malloc(sizeof(fd_handler));
    masterSocketHandler->handle_read = masterSocketHandle;
    masterSocketHandler->handle_write = NULL;
    masterSocketHandler->handle_block = NULL;
    masterSocketHandler->handle_close = masterSocketHandleClose;

    // Add the master socket to the managed fds.
    // TODO: Review this
    selector_status resultStatus;
    resultStatus = selector_register(selector, master_socket, masterSocketHandler, OP_READ, (void *)&address);
    if(resultStatus != SELECTOR_SUCCESS){
        printf("Error in master socket: %s", selector_error(resultStatus));
    }


    // Accept the incoming connection
    addrlen = sizeof(address);
    printf("Waiting for connections on socket %d\n", master_socket);

    // Forever cicle until fatal error.
    while (TRUE)
    {
        // Wait for activiy of one of the sockets:
        // -Master Socket --> New connection.
        // -Child Sockets --> Read or write operation
        resultStatus = selector_select(selector);
        if(resultStatus != SELECTOR_SUCCESS){
            printf("Error awaiting for select: %s", selector_error(resultStatus));
        }
        
        // ----------------------------------TODO--------------------------------
        // From here on, its not implemented with selector.

        // Hay algo para escribir?
        // Si está listo para escribir, escribimos. El problema es que a pesar de tener buffer para poder
        // escribir, tal vez no sea suficiente. Por ejemplo podría tener 100 bytes libres en el buffer de
        // salida, pero le pido que mande 1000 bytes.Por lo que tenemos que hacer un send no bloqueante,
        // verificando la cantidad de bytes que pudo consumir TCP.
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];
            if (sd <= master_socket) // Pregunta: por que esta este "parche" ?
                continue;

            if (FD_ISSET(sd, &writefds))
            {
                size_t bytesToSend = bufferWrite[i].read - bufferWrite[i].write;
                if (bytesToSend > 0)
                { // Puede estar listo para enviar, pero no tenemos nada para enviar
                    printf("Trying to send %zu bytes to socket %d\n", bytesToSend, sd);
                    size_t bytesSent = send(sd, bufferWrite[i].read, bytesToSend, MSG_DONTWAIT); // | MSG_NOSIGNAL
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
                            // TODO: VER COMO HACER PARA LIBERAR EL BUFFER BIEN
                            /*
                            free(bufferWrite[i].data); 
                            bufferWrite[i].buffer = NULL;
                            bufferWrite[i].from = bufferWrite[i].len = 0;
                            */
                            bufferWrite[i].read += bytesSent;
                            buffer_compact(&(bufferWrite[i]));
                            FD_CLR(sd, &writefds);
                        }
                        else
                        {
                            bufferWrite[i].read += bytesSent;
                            buffer_compact(&(bufferWrite[i]));
                        }
                    }
                }
            }
        }

        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++)
        {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds))
            {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read(sd, received, BUFFERSIZE)) <= 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;

                    FD_CLR(sd, &writefds);
                    // Limpiamos el buffer asociado, para que no lo "herede" otra sesión
                    // TODO: hay codigo repetido
                    bufferWrite[i].read = bufferWrite[i].write;
                    buffer_compact(&(bufferWrite[i]));
                }
                else
                {
                    printf("Received %zu bytes from socket %d\n", valread, sd);

                    render_to_state(received, i, val_read, &buff);
                }
            }
        }
    }

    return 0;
}


/*  Handle for the master socket.
    Used to handle new connections.
*/
void masterSocketHandle(struct selector_key *key){

    int newSocket;
    struct sockaddr * address = (struct sockaddr *) key->data;
    socklen_t socklen = (socklen_t) sizeof(*address);

    // Accpet the new socket.
    if (newSocket = accept(newSocket, address, &socklen) < 0){
            perror("Error: Error accepting new socket\n");
            exit(EXIT_FAILURE);
        }

        // Inform user of socket number - used in send and receive commands
        printf("New connection , socket fd is %d , ip is : %s , port : %d \n", newSocket, inet_ntoa(address->sin_addr), ntohs(address->sin_port));

        // Handler for the slave sockets.
        // Create socket handler for the master socket
        fd_handler * slaveSocketHandler = malloc(sizeof(fd_handler));
        slaveSocketHandler->handle_read = slaveSocketHandleRead;
        slaveSocketHandler->handle_write = slaveSocketHandleWrite;
        slaveSocketHandler->handle_block = slaveSocketHandleBlock;
        slaveSocketHandler->handle_close = masterSocketHandleClose;

        // Register the new file descriptor.
        selector_status resultStatus;
        resultStatus = selector_register(key->s, newSocket, slaveSocketHandler, OP_READ + OP_WRITE, NULL);
        if(resultStatus != SELECTOR_SUCCESS){
            printf("Error: %s while creating registering new fd", selector_error(resultStatus));
        }
}

// TODO: implement this
void masterSocketHandleClose(struct selector_key *key){

}

// TODO: Implement
void slaveSocketHandleRead(struct selector_key *key){

}

// TODO: Implement
void slaveSocketHandleWrite(struct selector_key *key){

}

void slaveSocketHandleBlock(struct selector_key *key){

}
void slaveSocketHandleClose(struct selector_key *key){

}

void init_socks_state(int i)
{

    socks_state[i] = malloc(sizeof(socks_state));

    if (socks_state[i] == NULL)
    {
        perror("state machine malloc failure");
        exit(EXIT_FAILURE);
    }

    socks_state[i]->stm->state = HELLO_READ;
    socks_state[i]->client.hello.parser = newHelloParser();
}

void free_socks_state(int i)
{
    int i = 0;
    free(socks_state[i]);
}

void render_to_state(char *received, int sock_num, int valread, buffer *b)
{
    int errored = 0;
    switch (socks_state[sock_num]->stm->state)
    {
    case HELLO_READ:
        for (int i = 0; i < valread; i++)
        {
            buffer_write(b, received[i]);
        }

        HelloState hs;
        hs = helloConsumeMessage(b, socks_state[sock_num]->client.hello.parser, &errored);

        if (errored)
        {
            perror("Error during hello parsing");
            exit(EXIT_FAILURE);
        }

        if (hs == DONE)
        {

            socks_state[sock_num]->stm = HELLO_WRITE;
            freeHelloParser(socks_state[sock_num]->client.hello.parser);
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
