#include "SCTPSocksv5.h"


int sctpSocket(int port);

int main(){
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[MAX_SOCKETS] , max_clients = MAX_SOCKETS , activity, i , sd;
    long val_read;
    int max_sd;
    struct sockaddr_in address;

    char buffer[BUFFERSIZE + 1]; //data buffer of 2K

    //set of socket descriptors
    fd_set readfds;

    //a writing buffer for each socket so that writing is not blocking
    struct buffer bufferWrite[MAX_SOCKETS];
    memset(bufferWrite, 0, sizeof bufferWrite);

    //initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }

     //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , IPPROTO_SCTP)) == 0) 
    {
        //log(FATAL, "socket failed");
        exit(EXIT_FAILURE);
    }


    //we could allow multiple connections to master

     //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );

        //bind the socket to localhost port 1080
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        //log(FATAL, "bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(master_socket, MAX_PENDING_CONNECTIONS) < 0)
    {
        //log(FATAL, "listen");
        exit(EXIT_FAILURE);
    }

    int sctpSocket = sctp_socket(PORT);

    if ( sctpSocket < 0) {
        exit(EXIT_FAILURE);
    }






}

    int sctpSocket(int port) {
    
    int sock;
    struct sockaddr_in serverAddr;
    if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        //log(ERROR, "UDP socket creation failed, errno: %d %s", errno, strerror(errno));
        return sock;
    }
    //log(DEBUG, "UDP socket %d created", sock);
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family    = AF_INET; // IPv4cle
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if ( bind(sock, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
    {
        //log(ERROR, "UDP bind failed, errno: %d %s", errno, strerror(errno));
        close(sock);
        return -1;
    }
    //log(DEBUG, "UDP socket bind OK ");
    
    return sock;
}