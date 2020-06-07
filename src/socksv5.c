
#include "../include/socksv5.h"

//#include "Utility.h"



int main() {
    int master_socket , addrlen , new_socket , client_socket[MAX_SOCKETS] , max_clients = MAX_SOCKETS , activity, i , sd;
    long val_read;
    int max_sd;
    struct sockaddr_in address;
    long valread;

     char buffer[BUFFERSIZE + 1]; //data buffer of 2K


    fd_set readfds;
    fd_set writefds;

    struct buffer bufferWrite[MAX_SOCKETS];
    memset(bufferWrite, 0, sizeof bufferWrite);


    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
    }

    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        printf("socket failed");
        //log(FATAL, "socket failed");
        exit(EXIT_FAILURE);
    }
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
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

     //accept the incoming connection
    addrlen = sizeof(address);
    printf("Waiting for connections on socket %d\n", master_socket);

    // Limpiamos el conjunto de escritura
    FD_ZERO(&writefds);

        while(TRUE) 
    {
        //clear the socket set
        FD_ZERO(&readfds);
  
        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
         
        //add child sockets to set
        for ( i = 0 ; i < max_clients ; i++) 
        {
            //socket descriptor
            sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }
  
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , &writefds , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) 
        {
            printf("select error");
        }
          
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
            //add new socket to array of sockets
            for (i = 0; i < max_clients; i++) 
            {
                //if position is empty
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                     
                    break;
                }
            }
        }
        
        // Hay algo para escribir?
        // Si está listo para escribir, escribimos. El problema es que a pesar de tener buffer para poder
        // escribir, tal vez no sea suficiente. Por ejemplo podría tener 100 bytes libres en el buffer de
        // salida, pero le pido que mande 1000 bytes.Por lo que tenemos que hacer un send no bloqueante,
        // verificando la cantidad de bytes que pudo consumir TCP.
        for(i =0; i < max_clients; i++) {
            sd = client_socket[i];
            if (sd<=master_socket)   // Pregunta: por que esta este "parche" ?
                continue;

            if (FD_ISSET(sd, &writefds)) {
                size_t bytesToSend = bufferWrite[i].len - bufferWrite[i].from;
                if (bytesToSend > 0) {  // Puede estar listo para enviar, pero no tenemos nada para enviar
                    printf("Trying to send %zu bytes to socket %d\n", bytesToSend, sd);
                    size_t bytesSent = send(sd, bufferWrite[i].buffer + bufferWrite[i].from,bytesToSend,  MSG_DONTWAIT); // | MSG_NOSIGNAL
                    printf("Sent %zu bytes\n", bytesSent);
                    
                    if ( bytesSent < 0) {
                        // Esto no deberia pasar ya que el socket estaba listo para escritura
                        // TODO: manejar el error
                        perror(" ");
                    } else {
                        size_t bytesLeft = bytesSent - bytesToSend;
                        
                        // Si se pudieron mandar todos los bytes limpiamos el buffer
                        // y sacamos el fd para el select
                        if ( bytesLeft == 0) {
                            free(bufferWrite[i].buffer);
                            bufferWrite[i].buffer = NULL;
                            bufferWrite[i].from = bufferWrite[i].len = 0;
                            FD_CLR(sd, &writefds);
                        } else {
                            bufferWrite[i].from += bytesSent;
                        }
                    }
                }
            }
        }
        
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
              
            if (FD_ISSET( sd , &readfds)) 
            {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read( sd , buffer, BUFFERSIZE )) <= 0)
                {
                    //Somebody disconnected , get his details and print
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    client_socket[i] = 0;
                    
                    FD_CLR(sd, &writefds);
                    // Limpiamos el buffer asociado, para que no lo "herede" otra sesión
                    // TODO: hay codigo repetido
                    free(bufferWrite[i].buffer);
                    bufferWrite[i].buffer = NULL;
                    bufferWrite[i].from = bufferWrite[i].len = 0;
                }
                else {
                    printf("Received %zu bytes from socket %d\n", valread, sd);
                    // activamos el socket para escritura y almacenamos en el buffer de salida
                    FD_SET(sd, &writefds);


                    /* receive the response */
                    //memset(response,0,sizeof(response));
                    //total = sizeof(response)-1;
                    //received = 0;
                    //bytes = read(sockfd,response+received,total - received); //have to do it while there is no more to read but it gets stucked



                    
                    // Tal vez ya habia datos en el buffer
                    // TODO: validar realloc != NULL
                    //bufferWrite[i].buffer = realloc(bufferWrite[i].buffer, bufferWrite[i].len + bytes);
                    //memcpy(bufferWrite[i].buffer + bufferWrite[i].len, response, bytes);
                    //bufferWrite[i].len += bytes;
                }
            }
        }
    }





    return 0;
}