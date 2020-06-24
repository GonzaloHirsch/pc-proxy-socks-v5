#include "socksv5.h"

// -------------- INTERNAL FUNCTIONS-----------------------------------

void renderToState(struct selector_key *key, char *received, int valread);

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

int main(const int argc, char *const *argv)
{
    // Parsing the command line arguments
    parse_args(argc, argv);

    printf("Starting server...\n");

    int master_socket = -1, master_socket_6 = -1, management_socket = -1, management_socket_6 = -1;

    fd_handler *management_socket6_handler = NULL, *management_socket_handler = NULL, *master_socket_handler = NULL, *master_socket6_handler = NULL;

    // Selector for concurrent connexions
    fd_selector selector = NULL;

    if (options->socks_family == AF_UNSPEC)
    {
        create_master_socket_all(&master_socket, &master_socket_6);
    }
    else if (options->socks_family == AF_INET)
    {
        master_socket = create_master_socket_4(&options->socks_addr_info);
    }
    else if (options->socks_family == AF_INET6)
    {
        master_socket_6 = create_master_socket_6(&options->socks_addr_info6);
    }

    if (options->mng_family == AF_UNSPEC)
    {
        create_management_socket_all(&management_socket, &management_socket_6);
    }
    else if (options->mng_family == AF_INET)
    {
        management_socket = create_management_socket_4(&options->mng_addr_info);
    }
    else if (options->mng_family == AF_INET6)
    {
        management_socket_6 = create_management_socket_6(&options->mng_addr_info6);
    }

    // ----------------- CREATING SIGNAL HANDLERS -----------------

    // Handling the SIGTERM and SIGINT signal, in order to make server stopping more clean and be able to free resources
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // ----------------- CREATE THE SELECTOR -----------------

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

    // Adding the command line users to our own user table
    int user_count;
    struct users user;
    for (user_count = 0; user_count < options->n_users; user_count++){
        user = options->users[user_count];
        create_user_proxy((uint8_t *)user.name, (uint8_t *)user.pass, strlen(user.name), strlen(user.pass));
    }

    // Initializing the metrics struct
    init_metrics();
    perror("Waiting for proxy connections on: \n");

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
            fprintf(stderr,"Error in master socket: %s", selector_error(ss_master));
        }
        fprintf(stderr,"  -IP: %s  PORT: %d \n", options->socks_addr, options->socks_port);

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
            fprintf(stderr,"Error in master socket: %s", selector_error(ss_master6));
        }
        fprintf(stderr,"  -IP: %s  PORT: %d \n", options->socks_addr_6, options->socks_port);
    }

    perror("Waiting for management connections on: \n");
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
            fprintf(stderr, "Error in management socket: %s", selector_error(ss_management));
        }
        fprintf(stderr, "  -IP: %s  PORT: %d \n", options->mng_addr, options->mng_port);
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
            fprintf(stderr, "Error in management socket: %s", selector_error(ss_management6));
        }
        fprintf(stderr, "  -IP: %s  PORT: %d \n", options->mng_addr_6, options->mng_port);

    }

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
            fprintf(stderr ,"Error awaiting for select: %s\n", selector_error(ss));
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