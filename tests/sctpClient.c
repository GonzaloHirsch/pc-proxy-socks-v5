#include "sctpClient.h"

int main(int argc, char *argv[])
{
    int serverSocket, in, i, ret, flags;
    struct sockaddr_in server;
    struct sctp_status status;
    char buffer[MAX_BUFFER + 1];
    int datalen = 0;

    // Creating the socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (serverSocket == -1)
    {
        printf("Error creating the socket\n");
        perror("socket()");
        exit(1);
    }

    // Setting the struct to 0
    bzero((void *)&server, sizeof(server));
    // Adding the server
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connecting to the server
    ret = connect(serverSocket, (struct sockaddr *)&server, sizeof(server));
    if (ret == -1)
    {
        printf("Connection failed\n");
        perror("connect()");
        close(serverSocket);
        exit(1);
    }

    /* VER(1), ULEN(1), USER(5 -> gonza), PLEN(1), PASS(8 -> admin123)*/
    uint8_t data[] = {
        0x01, 0x05, 0x67, 0x6F, 0x6E, 0x7A, 0x62, 0x08, 0x61, 0x64, 0x6D, 0x69, 0x6E, 0x31, 0x32, 0x33};

    ret = sctp_sendmsg(serverSocket, (void *)data, N(data), NULL, 0, 0, 0, 0, 0, 0);
    while (1)
        ;

    /*
    fgets(buffer, MAX_BUFFER, stdin);
    buffer[strcspn(buffer, "\r\n")] = 0;
    datalen = strlen(buffer);

    
    ret = sctp_sendmsg(serverSocket, (void *)buffer, (size_t)datalen,
                       NULL, 0, 0, 0, 0, 0, 0);
    if (ret == -1)
    {
        printf("Error in sctp_sendmsg\n");
        perror("sctp_sendmsg()");
    }
    else
        printf("Successfully sent %d bytes data to server\n", ret);

    close(connSock);
*/
    return 0;
}