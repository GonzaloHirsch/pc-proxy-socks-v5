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

    // -------------------------------- LOGIN --------------------------------

    /* VER(1), ULEN(1), USER(5 -> gonza), PLEN(1), PASS(8 -> admin123)*/
    uint8_t data_good[] = {
        0x01, 0x05, 0x67, 0x6F, 0x6E, 0x7A, 0x61, 0x08, 0x61, 0x64, 0x6D, 0x69, 0x6E, 0x31, 0x32, 0x33};

    uint8_t data_bad[] = {
        0x01, 0x05, 0x67, 0x6F, 0x6E, 0x7A, 0x62, 0x08, 0x61, 0x64, 0x6D, 0x69, 0x6E, 0x31, 0x32, 0x33};

    // Sending login request
    ret = sctp_sendmsg(serverSocket, (void *)data_good, N(data_good), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error sending login \n");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- LOGIN RESPONSE --------------------------------

    uint8_t login_response_buffer[2];
    struct sctp_sndrcvinfo sndrcvinfo;
    size_t login_count = N(login_response_buffer);
    int recv_flags = 0;

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, login_response_buffer, login_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        printf("Error receiving login response\n");
        close(serverSocket);
        exit(0);
    }

    if (login_response_buffer[0] == 0x01)
    {
        printf("VERSION OK\n");
    }
    else
    {
        printf("BAD VERSION\n");
    }

    if (login_response_buffer[1] == 0x00)
    {
        printf("LOGIN OK\n");
    }
    else
    {
        printf("BAD LOGIN\n");
    }

    // -------------------------------- USER LIST REQUEST --------------------------------

    uint8_t user_list[] = {0x01, 0x01};

    // Sending login request
    ret = sctp_sendmsg(serverSocket, (void *)user_list, N(user_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error sending user list \n");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- USER LIST RESPONSE --------------------------------

    uint8_t user_list_buffer[2048];
    size_t user_list_count = N(user_list_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, user_list_buffer, user_list_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        printf("Error receiving login response\n");
        close(serverSocket);
        exit(0);
    }

    if (user_list_buffer[0] == 0x01)
    {
        printf("TYPE OK\n");
    }
    else
    {
        printf("BAD TYPE\n");
    }

    if (user_list_buffer[1] == 0x01)
    {
        printf("CMD OK\n");
    }
    else
    {
        printf("BAD CMD\n");
    }

    printf("Given %d users %s\n", user_list_buffer[2], user_list_buffer + 3);

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