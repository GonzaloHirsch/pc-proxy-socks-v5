    #include <sys/types.h>
       #include <stdio.h>
       #include <stdlib.h>
       #include <unistd.h>
       #include <string.h>
       #include <sys/socket.h>
       #include <netdb.h>


       #define BUF_SIZE 500

void error(const char *msg) { perror(msg); exit(0); }

int main(int argc, char ** argv){

           /* first where are we going to send it? */
    int portno = atoi(argv[2])>0?atoi(argv[2]):80;
    char *host = argv[1];

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total, message_size;
    char *message, response[4096];
             /* What are we going to send? */

    

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) error("ERROR, no such host");

    /* send the request */
    printf("%s", host);
    printf("%s", argv[1]);

    char peer0_0[] = { /* Packet 57 */
0x47, 0x45, 0x54, 0x20, 0x2f, 0x20, 0x48, 0x54, 
0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31, 0x0d, 0x0a, 
0x48, 0x6f, 0x73, 0x74, 0x3a, 0x20, 0x67, 0x6f, 
0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 
0x0d, 0x0a, 0x55, 0x73, 0x65, 0x72, 0x2d, 0x41, 
0x67, 0x65, 0x6e, 0x74, 0x3a, 0x20, 0x63, 0x75, 
0x72, 0x6c, 0x2f, 0x37, 0x2e, 0x36, 0x34, 0x2e, 
0x31, 0x0d, 0x0a, 0x41, 0x63, 0x63, 0x65, 0x70, 
0x74, 0x3a, 0x20, 0x2a, 0x2f, 0x2a, 0x0d, 0x0a, 
0x0d, 0x0a };

         /* What are we going to send? */
    //printf("Request:\n%s\n",peer0_0);
    printf("hola2");

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    /* send the request */
    total = strlen(peer0_0);
    sent = 0;
    do {
        bytes = write(sockfd,peer0_0+sent,total-sent);
        if (bytes < 0)
            error("ERROR writing message to socket");
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    /* receive the response */
    memset(response,0,sizeof(response));
    total = sizeof(response)-1;
    received = 0;
    bytes = read(sockfd,response+received,total - received); //have to do it while there is no more to read but it gets stucked



    /* close the socket */
    close(sockfd);

    /* process response */
    printf("Response:\n%s\n",response);

    //free(message);
    return 0;


}