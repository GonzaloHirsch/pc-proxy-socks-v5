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

    printf("\nLOGGING IN\n\n");

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

    // -------------------------------- USER CREATE REQUEST --------------------------------

    printf("\nCREATING USER\n\n");

    uint8_t user_create_list[] = {0x01, 0x02, 0x01, 0x08, 0x6E, 0x65, 0x77, 0x61, 0x64, 0x6D, 0x69, 0x6E, 0x07, 0x6E, 0x65, 0x77, 0x70, 0x61, 0x73, 0x73};

    // Sending login request
    ret = sctp_sendmsg(serverSocket, (void *)user_create_list, N(user_create_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error creating user \n");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- USER CREATE RESPONSE --------------------------------

    uint8_t user_create_buffer[2048];
    size_t user_create_count = N(user_create_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, user_create_buffer, user_create_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        printf("Error receiving user creation response\n");
        close(serverSocket);
        exit(0);
    }

    if (user_create_buffer[0] == 0x01)
    {
        printf("TYPE OK\n");
    }
    else
    {
        printf("BAD TYPE\n");
    }

    if (user_create_buffer[1] == 0x02)
    {
        printf("CMD OK\n");
    }
    else
    {
        printf("BAD CMD\n");
    }

    if (user_create_buffer[2] == 0x00)
    {
        printf("STATUS OK\n");
    }
    else
    {
        printf("BAD STATUS\n");
    }

    if (user_create_buffer[3] == 0x01)
    {
        printf("VERSION OK\n");
    }
    else
    {
        printf("BAD VERSION\n");
    }

    // -------------------------------- USER LIST REQUEST --------------------------------

    printf("\nLISTING USERS\n\n");

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
        printf("Error receiving user list response\n");
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

    if (user_list_buffer[2] == 0x00)
    {
        printf("STATUS OK\n");
    }
    else
    {
        printf("BAD STATUS\n");
    }

    printf("Given %d users %s\n", user_list_buffer[3], user_list_buffer + 4);

    // -------------------------------- METRICS LIST REQUEST --------------------------------

    printf("\nLISTING METRICS\n\n");

    uint8_t metrics_list[] = {0x02, 0x01};

    // Sending login request
    ret = sctp_sendmsg(serverSocket, (void *)metrics_list, N(metrics_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error sending metrics list \n");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- METRICS LIST RESPONSE --------------------------------

    uint8_t metrics_list_buffer[2048];
    size_t metrics_list_count = N(metrics_list_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, metrics_list_buffer, metrics_list_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        printf("Error receiving metrics list response\n");
        close(serverSocket);
        exit(0);
    }

    if (metrics_list_buffer[0] == 0x02)
    {
        printf("TYPE OK\n");
    }
    else
    {
        printf("BAD TYPE\n");
    }

    if (metrics_list_buffer[1] == 0x01)
    {
        printf("CMD OK\n");
    }
    else
    {
        printf("BAD CMD\n");
    }

    if (metrics_list_buffer[2] == 0x00)
    {
        printf("STATUS OK\n");
    }
    else
    {
        printf("BAD STATUS\n");
    }

    if (metrics_list_buffer[3] == 0x03)
    {
        printf("COUNT OK\n");
    }
    else
    {
        printf("BAD COUNT\n");
    }

    uint64_t bytes = ntoh64(metrics_list_buffer + 4);
    uint32_t hist = ntoh32(metrics_list_buffer + 4 + 8);
    uint32_t curr = ntoh32(metrics_list_buffer + 4 + 8 + 4);

    printf("Bytes %lu, historic %u, current %u\n", bytes, hist, curr);

    while(1){}

    close(serverSocket);

    return 0;
}