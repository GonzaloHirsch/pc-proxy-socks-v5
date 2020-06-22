#include "sctpClient.h"

#include "sctpClient_Users.h"
#include "sctpClient_Metrics.h"
#include "sctpClient_Configs.h"

#include "sctpArgs.h"

static bool end = false;
int serverSocket;

/**
 * Handler for an interruption signal, just changes the server state to finished
 * @param signal integer representing the signal sent
 * 
 * */
static void signal_handler(const int signal)
{
    printf("Received signal %d, exiting now", signal);
    end = true;
}

void die_with_message(char *msg)
{
    perror(msg);
    free_memory();
    close(serverSocket);
    exit(0);
}

static void greeting()
{
    printf("\n\nBienvenido al cliente para nuestro servidor");
}

static void print_option_title(int option)
{
    switch (option)
    {
    case OPT_EXIT:
        printf("Opción %d - Desconectarse\n\n", option);
        break;
    case OPT_SHOW_METRICS:
        printf("Opción %d - Mostrar Metricas\n\n", option);
        break;
    case OPT_LIST_USERS:
        printf("Opción %d - Listar Usuarios\n\n", option);
        break;
    case OPT_CREATE_USER:
        printf("Opción %d - Crear Usuario\n\n", option);
        break;
    case OPT_SHOW_CONFIGS:
        printf("Opción %d - Mostrar Configuraciones\n\n", option);
        break;
    case OPT_EDIT_CONFIG:
        printf("Opción %d - Editar Configuración\n\n", option);
        break;
    default:
        break;
    }
}

/**
 * Determines the option to be chosen
*/
static int show_options()
{
    printf("Posibles comandos para interactuar:\n"
           "1 - Desconectarse\n"
           "2 - Listar usuarios\n"
           "3 - Crear usuario\n"
           "4 - Mostrar métricas\n"
           "5 - Mostrar configuraciones\n"
           "6 - Editar configuración\n");

    printf("Elegir un número de comando para interactuar: ");
    int i;
    int result = scanf("%d", &i);

    if (result == EOF)
    {
        return -1;
    }
    else if (result == 0)
    {
        return -1;
    }

    return i;
}

void handle_undefined_command()
{
    printf("Opción desconocida, por favor elija otra");
}

void handle_invalid_value()
{
    printf("Valor inválido, por favor ingrese otro");
}

static void handle_exit()
{
    printf("Cerrando conexión...");
    close(serverSocket);
    free_memory();
}

int main(int argc, char *argv[])
{
    /* Parsing options - setting up proxy */
    parse_args(argc, argv);

    int ret;
    bool try_again = false;

    // If no address given, try both
    if (args->mng_addr_info == NULL)
    {
        struct sockaddr_in server;
        memset(&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        server.sin_port = htons(args->mng_port);
        server.sin_addr.s_addr = htons(INADDR_ANY);

        // Creating the socket
        serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
        if (serverSocket == -1)
        {
            try_again = true;
        }

        // Connecting to the server
        ret = connect(serverSocket, (struct sockaddr *)&server, sizeof(server));
        if (ret == -1)
        {
            try_again = true;
        }

        if (try_again)
        {
            // Address for IPv6 socket binding
            struct sockaddr_in6 server6;
            memset(&server6, 0, sizeof(server6));
            server6.sin6_family = AF_INET6;
            server6.sin6_addr = in6addr_any;
            server6.sin6_port = htons(args->mng_port);

            // Creating the socket
            serverSocket = socket(AF_INET6, SOCK_STREAM, IPPROTO_SCTP);
            if (serverSocket == -1)
            {
                printf("Error creating the socket\n");
                perror("socket()");
                free_memory();
                exit(1);
            }

            // Connecting to the server
            ret = connect(serverSocket, (struct sockaddr *)&server6, sizeof(server6));
            if (ret == -1)
            {
                printf("Connection failed\n");
                free_memory();
                die_with_message("connect()");
            }
        }
    }
    else
    {
        // Creating the socket
        serverSocket = socket(args->mng_family, SOCK_STREAM, IPPROTO_SCTP);
        if (serverSocket == -1)
        {
            printf("Error creating the socket\n");
            perror("socket()");
            free_memory();
            exit(1);
        }

        if (args->mng_family == AF_INET6)
        {
            struct sockaddr_in6 * adr_6 = (struct sockaddr_in6 *)args->mng_addr_info;
            // Connecting to the server
            ret = connect(serverSocket, (struct sockaddr *)adr_6, sizeof(*adr_6));
            if (ret == -1)
            {
                printf("Connection failed\n");
                free_memory();
                die_with_message("connect()");
            }
        }
        else
        {
            struct sockaddr_in * adr = (struct sockaddr_in *)args->mng_addr_info;
            // Connecting to the server
            ret = connect(serverSocket, (struct sockaddr *)adr, sizeof(*adr));
            if (ret == -1)
            {
                printf("Connection failed\n");
                free_memory();
                die_with_message("connect()");
            }
        }
    }

    // Handling the SIGTERM and SIGINT signal, in order to make server stopping more clean and be able to free resources
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    greeting();

    // -------------------------------- LOGIN --------------------------------

    uint8_t username[255];
    uint8_t password[255];

    while ((ret = try_log_in(username, password)) == 0)
    {
        // Nothing
    }

    // -------------------------------- INTERACTIVE OPTIONS --------------------------------

    int option;

    while (!end)
    {
        option = show_options();

        DIVIDER

        print_option_title(option);

        switch (option)
        {
        case OPT_EXIT:
            handle_exit();
            end = true;
            break;
        case OPT_SHOW_METRICS:
            handle_show_metrics();
            break;
        case OPT_SHOW_CONFIGS:
            handle_show_configs();
            break;
        case OPT_LIST_USERS:
            handle_list_users();
            break;
        case OPT_CREATE_USER:
            handle_create_user();
            break;
        case OPT_EDIT_CONFIG:
            handle_edit_config();
            break;
        default:
            handle_undefined_command();
            break;
        }

        DIVIDER
    }

    return 0;
}