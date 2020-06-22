#include "sctpClient_Users.h"

static int recv_flags = 0;
static struct sctp_sndrcvinfo sndrcvinfo;

static void remove_newline_if_present(uint8_t *buff)
{
    if (buff[strlen((const char *)buff) - 1] == '\n')
    {
        buff[strlen((const char *)buff) - 1] = '\0';
    }
}

int try_log_in(uint8_t *username, uint8_t *password)
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
        die_with_message("Error enviando login");
    }

    // -------------------------------- LOGIN RESPONSE --------------------------------

    uint8_t login_response_buffer[2];
    size_t login_count = N(login_response_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, login_response_buffer, login_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        die_with_message("Error recibiendo información de login");
    }
    else if (ret != 2)
    {
        die_with_message("Longitud inesperada");
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

void handle_create_user()
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
        die_with_message("Creando usuario");
    }

    // -------------------------------- USER CREATE RESPONSE --------------------------------

    uint8_t user_create_buffer[2048];
    size_t user_create_count = N(user_create_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, user_create_buffer, user_create_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        die_with_message("Recibiendo respuesta del servidor");
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

void handle_list_users()
{
    // -------------------------------- USER LIST REQUEST --------------------------------

    uint8_t user_list[] = {0x01, 0x01};

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)user_list, N(user_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        die_with_message("Enviando pedido al servidor");
    }

    // -------------------------------- USER LIST RESPONSE --------------------------------

    uint8_t user_list_buffer[255 * 260];
    size_t user_list_count = N(user_list_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, user_list_buffer, user_list_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        die_with_message("Recibiendo respuesta del servidor");
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
    uint32_t expected_users = ntoh32(user_list_buffer + 3);
    int response_index = 7; // Index to start reading users
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
                perror("Error allocating memory");
                int j;
                for (j = 0; j < processed_users; j++)
                {
                    free(users[j]);
                }
                free(users);
                return;
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
        perror("Error allocating memory");
        int j;
        for (j = 0; j < processed_users; j++)
        {
            free(users[j]);
        }
        free(users);
        return;
    }
    // Copy the data from the buffer into the array of pointers into the pointer
    memcpy(users[processed_users], buff, buff_index);
    // Add the 0 at the end of the string
    users[processed_users][buff_index] = 0x00;

    printf("Los usuarios son: \n");

    uint32_t i;
    for (i = 0; i < expected_users; i++)
    {
        // Printing the user
        if (i == expected_users - 1)
        {
            printf("%u - %s", i + 1, (const char *)users[i]);
        }
        else
        {
            printf("%u - %s\n", i + 1, (const char *)users[i]);
        }
        // Freeing that user
        free(users[i]);
    }

    free(users);
}