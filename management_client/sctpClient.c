#include "sctpClient.h"

#define DIVIDER printf("\n\n-------------------------------------------------------\n\n");
#define SUBDIVIDER printf("\n\n---------------------------\n\n");

static int serverSocket, recv_flags = 0;
struct sctp_sndrcvinfo sndrcvinfo;

static void greeting(){
    printf("\n\nBienvenido al cliente para nuestro servidor");
}

static void send_edit_config(Configs conf, uint8_t *data, ssize_t data_len);

static void print_option_title(int option)
{
    switch (option)
    {
    case OPT_EXIT:
        printf("Option %d - Desconectarse\n\n", option);
        break;
    case OPT_SHOW_METRICS:
        printf("Option %d - Mostrar Metricas\n\n", option);
        break;
    case OPT_LIST_USERS:
        printf("Option %d - Listar Usuarios\n\n", option);
        break;
    case OPT_CREATE_USER:
        printf("Option %d - Crear Usuario\n\n", option);
        break;
    case OPT_SHOW_CONFIGS:
        printf("Option %d - Mostrar Configuraciones\n\n", option);
        break;
    case OPT_EDIT_CONFIG:
        printf("Option %d - Editar Configuración\n\n", option);
        break;
    default:
        break;
    }
}

static void remove_newline_if_present(uint8_t *buff)
{
    if (buff[strlen((const char *)buff) - 1] == '\n')
    {
        buff[strlen((const char *)buff) - 1] = '\0';
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

static int try_log_in(uint8_t *username, uint8_t *password)
{
    // -------------------------------- LOGIN --------------------------------
    DIVIDER
    printf("Necesitas estar logueado para usar el servidor\n");

    printf("Usuario: ");
    fgets((char *)username, 255, stdin);

    printf("Contraseña: ");
    fgets((char *)password, 255, stdin);

    // Removing trailing \n from username and password
    remove_newline_if_present(username);
    remove_newline_if_present(password);

    ssize_t login_data_size = 3 + strlen((const char *)username) + strlen((const char *)password);
    uint8_t login_data[login_data_size];
    login_data[0] = 0x01;
    login_data[1] = strlen((const char *)username);
    memcpy(login_data + 2, username, strlen((const char *)username));
    login_data[2 + strlen((const char *)username)] = strlen((const char *)password);
    memcpy(login_data + 3 + strlen((const char *)username), password, strlen((const char *)password));

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)login_data, N(login_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error enviando login\n");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- LOGIN RESPONSE --------------------------------

    uint8_t login_response_buffer[2];
    size_t login_count = N(login_response_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, login_response_buffer, login_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        printf("Error recibiendo información de login\n");
        close(serverSocket);
        exit(0);
    }
    else if (ret != 2)
    {
        printf("Longitud inesperada\n");
        close(serverSocket);
        exit(0);
    }

    if (login_response_buffer[0] == 0x01 && login_response_buffer[1] == 0x00)
    {
        printf("\nExito al entrar al servidor!");
        DIVIDER
        return 1;
    }
    else
    {
        printf("Error al iniciar sesión");
        return 0;
    }
}

static void handle_undefined_command()
{
    printf("Opción desconocida, por favor elija otra");
}

static void handle_invalid_value()
{
    printf("Valor inválido, por favor ingrese otro");
}

static void handle_exit()
{
    printf("Cerrando conexión");
    close(serverSocket);
}

static void handle_show_metrics()
{
    // -------------------------------- METRICS LIST REQUEST --------------------------------

    uint8_t metrics_list[] = {0x02, 0x01};

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)metrics_list, N(metrics_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error enviando request para mostrar metricas\n");
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
        printf("Error recibiendo respuesta para mostrar metricas\n");
        close(serverSocket);
        exit(0);
    }
    else if (ret != 20)
    {
        printf("Tamaño desconocido de respuesta\n");
        close(serverSocket);
        exit(0);
    }

    if (metrics_list_buffer[0] != 0x02)
    {
        perror("Tipo diferente al esperado");
        return;
    }

    if (metrics_list_buffer[1] != 0x01)
    {
        perror("Comando diferente al esperado");
    }

    if (metrics_list_buffer[2] != 0x00)
    {
        perror("Status de error");
    }

    if (metrics_list_buffer[3] != 0x03)
    {
        perror("Cantidad de metricas inesperada");
    }

    uint64_t bytes = ntoh64(metrics_list_buffer + 4);
    uint32_t hist = ntoh32(metrics_list_buffer + 4 + 8);
    uint32_t curr = ntoh32(metrics_list_buffer + 4 + 8 + 4);

    printf("Cantidad de bytes transferidos: %lu\n", bytes);
    printf("Conexiones históricas: %u\n", hist);
    printf("Conexiones concurrentes: %u", curr);
}

static void handle_show_configs()
{
    // -------------------------------- CONFIGS LIST REQUEST --------------------------------

    uint8_t configs_list[] = {0x03, 0x01};

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)configs_list, N(configs_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error enviando request para mostrar configuraciones\n");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- CONFIGS LIST RESPONSE --------------------------------

    uint8_t configs_list_buffer[2048];
    size_t configs_list_count = N(configs_list_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, configs_list_buffer, configs_list_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        printf("Error recibiendo respuesta para mostrar configuraciones\n");
        close(serverSocket);
        exit(0);
    }
    else if (ret != 11)
    {
        printf("Tamaño desconocido de respuesta\n");
        close(serverSocket);
        exit(0);
    }

    if (configs_list_buffer[0] != 0x03)
    {
        perror("Tipo diferente al esperado");
    }

    if (configs_list_buffer[1] != 0x01)
    {
        perror("Comando diferente al esperado");
    }

    if (configs_list_buffer[2] != 0x00)
    {
        perror("Status de error");
    }

    if (configs_list_buffer[3] != 0x04)
    {
        perror("Cantidad de configuraciones inesperada");
    }

    uint16_t socks5_buff_len = ntoh16(configs_list_buffer + 4);
    uint16_t sctp_buff_len = ntoh16(configs_list_buffer + 6);
    uint16_t doh_buff_len = ntoh16(configs_list_buffer + 8);
    uint8_t timeout = configs_list_buffer[10];

    printf("Tamaño del buffer del proxy socks5: %d\n", socks5_buff_len);
    printf("Tamaño del buffer del socket de management (bytes): %d\n", sctp_buff_len);
    printf("Tamaño del buffer de doh (bytes): %d\n", doh_buff_len);
    printf("Tiempo de timeout (segundos): %d", timeout);
}

static void handle_create_user()
{
    // -------------------------------- USER CREATE REQUEST --------------------------------

    printf("Ingrese el nombre del nuevo usuario: ");

    uint8_t username[255];
    int result = scanf("%s", username);

    if (result == EOF)
    {
        perror("Leyendo nombre de usuario");
        return;
    }
    else if (result == 0)
    {
        perror("Leyendo nombre de usuario");
        return;
    }

    printf("Ingrese la contraseña del nuevo usuario: ");

    uint8_t password[255];
    result = scanf("%s", password);

    if (result == EOF)
    {
        perror("Leyendo contraseña");
        return;
    }
    else if (result == 0)
    {
        perror("Leyendo contraseña");
        return;
    }

    // Creating the data to be sent
    uint8_t user_create_data[5 + strlen((const char *)username) + strlen((const char *)password)];
    user_create_data[0] = 0x01;                                                                              // TYPE
    user_create_data[1] = 0x02;                                                                              // CMD
    user_create_data[2] = 0x01;                                                                              // VERSION
    user_create_data[3] = (uint8_t)strlen((const char *)username);                                           // ULEN
    memcpy(user_create_data + 4, username, strlen((const char *)username));                                  // USERNAME
    user_create_data[4 + strlen((const char *)username)] = (uint8_t)strlen((const char *)password);          // PLEN
    memcpy(user_create_data + 5 + strlen((const char *)username), password, strlen((const char *)password)); // PASSWORD

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)user_create_data, N(user_create_data), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        perror("Creando usuario");
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
        perror("Recibiendo respuesta del servidor");
        close(serverSocket);
        exit(0);
    }

    // Checking TYPE byte
    if (user_create_buffer[0] != 0x01)
    {
        perror("Tipo diferente al esperado");
        return;
    }

    // Checking CMD byte
    if (user_create_buffer[1] != 0x02)
    {
        perror("Comando diferente al esperado");
        return;
    }

    // Checking STATUS byte
    if (user_create_buffer[2] != 0x00)
    {
        perror("Status de error");
        return;
    }

    if (user_create_buffer[3] != 0x01)
    {
        perror("Versión diferente a la esperada");
        return;
    }

    printf("\nUsuario creado con éxito");
}

static void handle_list_users()
{
    // -------------------------------- USER LIST REQUEST --------------------------------

    uint8_t user_list[] = {0x01, 0x01};

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)user_list, N(user_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        perror("Enviando pedido al servidor");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- USER LIST RESPONSE --------------------------------

    uint8_t user_list_buffer[255 * 260];
    size_t user_list_count = N(user_list_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, user_list_buffer, user_list_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        perror("Recibiendo respuesta del servidor");
        close(serverSocket);
        exit(0);
    }

    // Checking TYPE byte
    if (user_list_buffer[0] != 0x01)
    {
        perror("Tipo diferente al esperado");
        return;
    }

    // Checking CMD byte
    if (user_list_buffer[1] != 0x01)
    {
        perror("Comando diferente al esperado");
        return;
    }

    // Checking STATUS byte
    if (user_list_buffer[2] != 0x00)
    {
        perror("Status de error");
        return;
    }

    int processed_users = 0;
    int expected_users = user_list_buffer[3];
    int response_index = 4; // Index to start reading users
    uint8_t **users = malloc(sizeof(uint8_t *) * expected_users);
    if (users == NULL)
    {
        printf("I had no memory\n");
        return;
    }

    // Data to hold the new user being read
    uint8_t buff[255];
    int buff_index = 0;

    // Iterate while the user index is less than the expected index and the index of the buffer is less than the given size by the recv
    while (processed_users < expected_users && response_index < ret)
    {
        // New user detected
        if (user_list_buffer[response_index] == 0x00)
        {
            // Malloc space for the data + \0
            users[processed_users] = malloc(sizeof(uint8_t) * (buff_index + 1));
            if (users[processed_users] == NULL)
            {
                printf("THIS IS F EMPTY\n");
            }
            // Copy the data from the buffer into the array of pointers into the pointer
            memcpy(users[processed_users], buff, buff_index);
            // Add the 0 at the end of the string
            users[processed_users][buff_index] = 0x00;
            // Updating the variables
            processed_users++;
            buff_index = 0;
        }
        else
        {
            buff[buff_index++] = user_list_buffer[response_index];
        }
        response_index++;
    }

    // Saving the last username
    users[processed_users] = malloc(sizeof(uint8_t) * (buff_index + 1));
    if (users[processed_users] == NULL)
    {
        printf("THIS IS F EMPTY\n");
    }
    // Copy the data from the buffer into the array of pointers into the pointer
    memcpy(users[processed_users], buff, buff_index);
    // Add the 0 at the end of the string
    users[processed_users][buff_index] = 0x00;

    printf("Los usuarios son: \n");

    int i;
    for (i = 0; i < expected_users; i++)
    {
        // Printing the user
        if (i == expected_users - 1){
            printf("%d - %s", i + 1, (const char *)users[i]);
        } else {
            printf("%d - %s\n", i + 1, (const char *)users[i]);
        }
        // Freeing that user
        free(users[i]);
    }

    free(users);
}

static int show_config_options()
{
    printf("Posibles configuraciones para editar:\n"
           "1 - Tamaño de buffer de proxy\n"
           "2 - Tamaño de buffer de management\n"
           "3 - Tamaño de buffer de DoH\n"
           "4 - Timeout\n"
           "100 - Volver para Atrás\n");

    printf("Elegir un número de configuración: ");
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

static bool get_16_bit_number(uint16_t *n)
{
    printf("Nuevo valor para configuración (0 - 65535): ");

    int i;
    int result = scanf("%d", &i);

    if (result == EOF)
    {
        return false;
    }
    else if (result == 0)
    {
        return false;
    }
    else if (i > 65535 || i < 0)
    {
        return false;
    }

    *n = i;

    return true;
}

static bool get_8_bit_number(uint8_t *n)
{
    printf("Nuevo valor para configuración (0 - 255): ");

    int i;
    int result = scanf("%d", &i);

    if (result == EOF)
    {
        return false;
    }
    else if (result == 0)
    {
        return false;
    }

    *n = i;

    return true;
}

static void handle_edit_config()
{
    int option;
    bool end = false;
    bool parseOk = false;
    union {
        uint8_t val8;
        uint16_t val16;
    } val;
    union {
        uint8_t data16[5];
        uint8_t data8[4];
    } data;

    while (!end)
    {
        option = show_config_options();

        SUBDIVIDER

        switch (option)
        {
        case CONF_DOH_BUFF:
            printf("Option %d - Tamaño de Buffer de DoH\n\n", option);
            parseOk = get_16_bit_number(&val.val16);
            if (!parseOk)
            {
                handle_invalid_value();
            }
            else
            {
                hton16(data.data16 + 3, val.val16);
                send_edit_config(option, data.data16, 5);
                end = true;
            }
            break;
        case CONF_SCTP_BUFF:
            printf("Option %d - Tamaño de Buffer de SCTP\n\n", option);
            parseOk = get_16_bit_number(&val.val16);
            if (!parseOk)
            {
                handle_invalid_value();
            }
            else
            {
                hton16(data.data16 + 3, val.val16);
                send_edit_config(option, data.data16, 5);
                end = true;
            }
            break;
        case CONF_SOCKS5_BUFF:
            printf("Option %d - Tamaño de Buffer de Proxy\n\n", option);
            parseOk = get_16_bit_number(&val.val16);
            if (!parseOk)
            {
                handle_invalid_value();
            }
            else
            {
                hton16(data.data16 + 3, val.val16);
                send_edit_config(option, data.data16, 5);
                end = true;
            }
            break;
        case CONF_TIMEOUT:
            printf("Option %d - Timeout\n\n", option);
            parseOk = get_8_bit_number(&val.val8);
            if (!parseOk)
            {
                handle_invalid_value();
            }
            else
            {
                data.data8[3] = val.val8;
                send_edit_config(option, data.data8, 4);
                end = true;
            }
            break;
        case CONF_EXIT:
            end = true;
            break;
        default:
            handle_undefined_command();
            break;
        }

        SUBDIVIDER
    }
}

static void send_edit_config(Configs conf, uint8_t *data, ssize_t data_len)
{
    // -------------------------------- CONFIGS EDIT REQUEST --------------------------------
    data[0] = 0x03;
    data[1] = 0x03;
    data[2] = conf;

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)data, data_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        printf("Error enviando request para editar configuracion\n");
        close(serverSocket);
        exit(0);
    }

    // -------------------------------- CONFIGS LIST RESPONSE --------------------------------

    uint8_t configs_edit_buffer[2048];
    size_t configs_edit_count = N(configs_edit_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, configs_edit_buffer, configs_edit_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        printf("Error recibiendo respuesta para editar configuracion\n");
        close(serverSocket);
        exit(0);
    }
    else if (ret != 4)
    {
        printf("Tamaño desconocido de respuesta\n");
        close(serverSocket);
        exit(0);
    }

    if (configs_edit_buffer[0] != 0x03)
    {
        perror("Tipo diferente al esperado");
        return;
    }

    if (configs_edit_buffer[1] != 0x03)
    {
        perror("Comando diferente al esperado");
        return;
    }

    if (configs_edit_buffer[2] != 0x00)
    {
        perror("Status de error");
        return;
    }

    if (configs_edit_buffer[3] != conf)
    {
        perror("Configuración inesperada");
        return;
    }

    printf("Configuración actualizada con éxito");
}

int main(int argc, char *argv[])
{
    // Args for the client
    struct sctpClientArgs *clientOptions = malloc(sizeof(struct sctpClientArgs));

    /* Parsing options - setting up proxy */
    parse_args(argc, argv, clientOptions);

    int ret;
    struct sockaddr_in server;

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
    server.sin_port = htons(clientOptions->mng_port);
    server.sin_addr.s_addr = inet_addr(clientOptions->mng_addr);

    // Connecting to the server
    ret = connect(serverSocket, (struct sockaddr *)&server, sizeof(server));
    if (ret == -1)
    {
        printf("Connection failed\n");
        perror("connect()");
        close(serverSocket);
        exit(1);
    }

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
    bool end = false;

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

    // Closing the connection
    close(serverSocket);

    return 0;
}