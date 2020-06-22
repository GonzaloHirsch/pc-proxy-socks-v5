#include "socket_creation.h"


int create_master_socket_6(struct sockaddr_in6 * addr)
{
    int master_socket6 = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (master_socket6 <= 0)
    {
        printf("socket6 failed");
        //log(FATAL, "socket failed");
        exit(EXIT_FAILURE);
    }

    int yes = 1;
    int opt = TRUE;

    if (setsockopt(master_socket6, SOL_IPV6, IPV6_V6ONLY, (void *)&yes, sizeof(yes)) < 0)
    {
        printf("Failed to set IPV6_V6ONLY\n");
        exit(EXIT_FAILURE);
    }

    // Setting the master socket to allow multiple connections, not required, just good habit
    if (setsockopt(master_socket6, SOL_IPV6, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
    {
        perror("ERROR: Failure setting setting IPv6 master socket\n");
        exit(EXIT_FAILURE);
    }

    // Binding the socket to localhost:1080
    if (bind(master_socket6, (struct sockaddr *)addr, sizeof(*addr)) < 0)
    {
        perror("ERROR: Failure binding IPv6 master socket\n");
        exit(EXIT_FAILURE);
    }

    // Checking if the socket is able to listen
    if (listen(master_socket6, MAX_PENDING_CONNECTIONS) < 0)
    {
        perror("ERROR: Failure listening IPv6 master socket\n");
        exit(EXIT_FAILURE);
    }

    return master_socket6;
}

int create_master_socket_4(struct sockaddr_in * addr)
{
    // Creating the server socket to listen
    int master_socket = socket(AF_INET, SOCK_STREAM, 0);

    int opt = TRUE;

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
    if (bind(master_socket, (struct sockaddr *)addr, sizeof(*addr)) < 0)
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

    return master_socket;
}

void create_master_socket_all(int *master_socket_4, int *master_socket_6)
{
    // Address for socket binding
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(options->socks_addr);
    address.sin_port = htons(options->socks_port);

    // Generating the IPv6 address structure
    struct in6_addr in6addr;
    inet_pton(AF_INET6, options->socks_addr_6, &in6addr);

    // Address for IPv6 socket binding
    struct sockaddr_in6 address6;
    address6.sin6_family = AF_INET6;
    address6.sin6_addr = in6addr;
    address6.sin6_port = htons(options->socks_port);

    *master_socket_4 = create_master_socket_4(&address);
    *master_socket_6 = create_master_socket_6(&address6);
}

int create_management_socket_6(struct sockaddr_in6 * addr)
{
    // Creating the server socket to listen
    int management_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);

    int yes = 1;

    if (management_socket <= 0)
    {
        printf("Management IPv6 socket creation failed");
        //log(FATAL, "socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(management_socket, SOL_IPV6, IPV6_V6ONLY, (void *)&yes, sizeof(yes)) < 0)
    {
        printf("Failed to set IPV6_V6ONLY\n");
        exit(EXIT_FAILURE);
    }

    // Struct for the streams configuration
    struct sctp_initmsg initmsg;
    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_num_ostreams = 1;
    initmsg.sinit_max_instreams = 1;
    initmsg.sinit_max_attempts = 4;

    // Setting the master socket to allow multiple connections, not required, just good habit
    // This setsockopt gives an error when used, if not used, the client still works
    if (setsockopt(management_socket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0)
    {
        perror("ERROR: Failure setting setting IPv6 management socket\n");
        exit(EXIT_FAILURE);
    }

    // Binding the socket to localhost
    if (bind(management_socket, (struct sockaddr *)addr, sizeof(*addr)) < 0)
    {
        perror("ERROR: Failure binding IPv6 management socket\n");
        exit(EXIT_FAILURE);
    }

    // Checking if the socket is able to listen
    if (listen(management_socket, MAX_PENDING_CONNECTIONS) < 0)
    {
        perror("ERROR: Failure listening IPv6 management socket\n");
        exit(EXIT_FAILURE);
    }

    return management_socket;
}

int create_management_socket_4(struct sockaddr_in * addr)
{
    // Creating the server socket to listen
    int management_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (management_socket <= 0)
    {
        printf("Management socket creation failed");
        //log(FATAL, "socket failed");
        exit(EXIT_FAILURE);
    }

    // Struct for the streams configuration
    struct sctp_initmsg initmsg;
    memset(&initmsg, 0, sizeof(initmsg));
    initmsg.sinit_num_ostreams = 1;
    initmsg.sinit_max_instreams = 1;
    initmsg.sinit_max_attempts = 4;

    // Setting the master socket to allow multiple connections, not required, just good habit
    // This setsockopt gives an error when used, if not used, the client still works
    if (setsockopt(management_socket, IPPROTO_SCTP, SCTP_INITMSG, &initmsg, sizeof(initmsg)) < 0)
    {
        perror("ERROR: Failure setting setting management socket\n");
        exit(EXIT_FAILURE);
    }

    // Binding the socket to localhost
    if (bind(management_socket, (struct sockaddr *)addr, sizeof(*addr)) < 0)
    {
        perror("ERROR: Failure binding management socket\n");
        exit(EXIT_FAILURE);
    }

    // Checking if the socket is able to listen
    if (listen(management_socket, MAX_PENDING_CONNECTIONS) < 0)
    {
        perror("ERROR: Failure listening management socket\n");
        exit(EXIT_FAILURE);
    }

    return management_socket;
}

void create_management_socket_all(int *management_socket_4, int *management_socket_6)
{
    // Address for sctp socket binding
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(options->mng_addr);
    address.sin_port = htons(options->mng_port);

    // Generating the IPv6 address structure
    struct in6_addr in6addr;
    inet_pton(AF_INET6, options->mng_addr_6, &in6addr);

    // Address for IPv6 socket binding
    struct sockaddr_in6 address6;
    address6.sin6_family = AF_INET6;
    address6.sin6_addr = in6addr;
    address6.sin6_port = htons(options->mng_port);

    *management_socket_4 = create_management_socket_4(&address);
    *management_socket_6 = create_management_socket_6(&address6);
}
