#include "socksv5.h"

// -------------- INTERNAL FUNCTIONS-----------------------------------

void renderToState(struct selector_key *key, char *received, int valread);

int create_management_socket_6(struct sockaddr_in6 * addr);
int create_management_socket_4(struct sockaddr_in * addr);
int create_master_socket_6(struct sockaddr_in6 * addr);
int create_master_socket_4(struct sockaddr_in * addr);

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

int main(const int argc, char *const *argv)
{
    // Parsing the command line arguments
    parse_args(argc, argv);

    printf("Starting server...\n");

    int master_socket = -1, master_socket_6 = -1, management_socket = -1, management_socket_6 = -1;

    fd_handler *management_socket6_handler, *management_socket_handler, *master_socket_handler, *master_socket6_handler;

    // Selector for concurrent connexions
    fd_selector selector = NULL;

    if (options->socks_family == AF_UNSPEC)
    {
        create_master_socket_all(&master_socket, &master_socket_6);
    }
    else if (options->socks_family == AF_INET)
    {
        master_socket = create_master_socket_4((struct sockaddr_in *) options->socks_addr_info);
    }
    else if (options->socks_family == AF_INET6)
    {
        master_socket_6 = create_master_socket_6((struct sockaddr_in6 *) options->socks_addr_info);
    }

    if (options->mng_family == AF_UNSPEC)
    {
        create_management_socket_all(&management_socket, &management_socket_6);
    }
    else if (options->mng_family == AF_INET)
    {
        management_socket = create_management_socket_4((struct sockaddr_in *) options->mng_addr_info);
    }
    else if (options->mng_family == AF_INET6)
    {
        management_socket_6 = create_management_socket_6((struct sockaddr_in6 *) options->mng_addr_info);
    }

    // ----------------- CREATING SIGNAL HANDLERS -----------------

    // Handling the SIGTERM and SIGINT signal, in order to make server stopping more clean and be able to free resources
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

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

    // Create user-pass table for authentication
    if (create_up_table() == -1)
    {
        perror("Error creating user pass table\n");
        abort();
    }

    // Create admin user-pass table for authentication
    if (create_up_admin_table() == -1)
    {
        perror("Error creating admin user pass table\n");
        abort();
    }

    // Initializing the metrics struct
    init_metrics();

    // Create socket handler for the master socket
    if (master_socket > 0)
    {
        master_socket_handler = malloc(sizeof(fd_handler));
        master_socket_handler->handle_read = socksv5_passive_accept;
        master_socket_handler->handle_write = NULL;
        master_socket_handler->handle_block = NULL;
        //TODO: define on close
        master_socket_handler->handle_close = NULL;

        // Register the master socket to the managed fds
        selector_status ss_master = selector_register(selector, master_socket, master_socket_handler, OP_READ, NULL);
        if (ss_master != SELECTOR_SUCCESS)
        {
            printf("Error in master socket: %s", selector_error(ss_master));
        }
    }

    if (master_socket_6 > 0)
    {
        // Create socket handler for IPv6 master socket
        master_socket6_handler = malloc(sizeof(fd_handler));
        master_socket6_handler->handle_read = socksv5_passive_accept;
        master_socket6_handler->handle_write = NULL;
        master_socket6_handler->handle_block = NULL;
        master_socket6_handler->handle_close = NULL;

        selector_status ss_master6 = selector_register(selector, master_socket_6, master_socket6_handler, OP_READ, NULL);
        if (ss_master6 != SELECTOR_SUCCESS)
        {
            printf("Error in master socket: %s", selector_error(ss_master6));
        }
    }

    if (management_socket > 0)
    {
        // Create socket handler for the master socket
        management_socket_handler = malloc(sizeof(fd_handler));
        management_socket_handler->handle_read = sctp_passive_accept;
        management_socket_handler->handle_write = NULL;
        management_socket_handler->handle_block = NULL;
        //TODO: define on close
        management_socket_handler->handle_close = NULL;

        // Register the SCTP socket to the managed fds
        selector_status ss_management = selector_register(selector, management_socket, management_socket_handler, OP_READ, NULL);
        if (ss_management != SELECTOR_SUCCESS)
        {
            printf("Error in management socket: %s", selector_error(ss_management));
        }
    }

    if (management_socket_6 > 0)
    {
        // Create socket handler for the master socket
        management_socket6_handler = malloc(sizeof(fd_handler));
        management_socket6_handler->handle_read = sctp_passive_accept;
        management_socket6_handler->handle_write = NULL;
        management_socket6_handler->handle_block = NULL;
        //TODO: define on close
        management_socket6_handler->handle_close = NULL;

        // Register the SCTP socket to the managed fds
        selector_status ss_management6 = selector_register(selector, management_socket_6, management_socket6_handler, OP_READ, NULL);
        if (ss_management6 != SELECTOR_SUCCESS)
        {
            printf("Error in management socket: %s", selector_error(ss_management6));
        }
    }

    // Accept the incoming connection
    printf("Waiting for connections on socket %d and management connections on socket %d\n", master_socket, management_socket);

    selector_status ss;

    // Cycle until a signal is received as finished
    while (!finished)
    {
        // printf("Awaiting connection\n");

        // Wait for activiy of one of the sockets:
        // -Master Socket --> New connection.
        // -Child Sockets --> Read or write operation
        ss = selector_select(selector);
        if (ss != SELECTOR_SUCCESS)
        {
            printf("Error awaiting for select: %s\n", selector_error(ss));
        }
    }

    // TODO: FREE ALL RESOURCES

    // Freeing the options for the args
    if (options != NULL)
    {
        free(options);
    }

    // Freeing the selector
    if (selector != NULL)
    {
        selector_destroy(selector);
    }
    selector_close();

    // Closing the sockets
    if (master_socket >= 0)
    {
        close(master_socket);
    }
    if (master_socket_6 >= 0)
    {
        close(master_socket_6);
    }
    if (management_socket >= 0)
    {
        close(management_socket);
    }
    if (management_socket_6 >= 0)
    {
        close(management_socket_6);
    }

    // Freeing the socket handlers
    if (master_socket_handler != NULL)
    {
        free(master_socket_handler);
    }
    if (master_socket6_handler != NULL)
    {
        free(master_socket6_handler);
    }
    if (management_socket_handler != NULL)
    {
        free(management_socket_handler);
    }
    if (management_socket6_handler != NULL)
    {
        free(management_socket6_handler);
    }

    // Freeing the users struct
    free_user_list();

    // Freeing the metrics struct
    destroy_metrics();

    return 0;
}