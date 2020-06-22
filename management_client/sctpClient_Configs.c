#include "sctpClient_Configs.h"

static int recv_flags = 0;
static struct sctp_sndrcvinfo sndrcvinfo;

void send_edit_config(Configs conf, uint8_t *data, ssize_t data_len);
int show_config_options();

bool get_16_bit_number(uint16_t *n)
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

bool get_8_bit_number(uint8_t *n)
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

void handle_show_configs()
{
    // -------------------------------- CONFIGS LIST REQUEST --------------------------------

    uint8_t configs_list[] = {0x03, 0x01};

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)configs_list, N(configs_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        die_with_message("Error enviando request para mostrar configuraciones");
    }

    // -------------------------------- CONFIGS LIST RESPONSE --------------------------------

    uint8_t configs_list_buffer[2048];
    size_t configs_list_count = N(configs_list_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, configs_list_buffer, configs_list_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        die_with_message("Error recibiendo respuesta para mostrar configuraciones");
    }
    else if (ret != 11)
    {
        die_with_message("Tamaño desconocido de respuesta");
    }

    if (configs_list_buffer[0] != 0x03)
    {
        perror("Tipo diferente al esperado");
        return;
    }

    if (configs_list_buffer[1] != 0x01)
    {
        perror("Comando diferente al esperado");
        return;
    }

    if (configs_list_buffer[2] != 0x00)
    {
        perror("Status de error");
        return;
    }

    if (configs_list_buffer[3] != 0x04)
    {
        perror("Cantidad de configuraciones inesperada");
        return;
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

int show_config_options()
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

void handle_edit_config()
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

void send_edit_config(Configs conf, uint8_t *data, ssize_t data_len)
{
    // -------------------------------- CONFIGS EDIT REQUEST --------------------------------
    data[0] = 0x03;
    data[1] = 0x03;
    data[2] = conf;

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)data, data_len, NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        die_with_message("Error enviando request para editar configuracion");
    }

    // -------------------------------- CONFIGS LIST RESPONSE --------------------------------

    uint8_t configs_edit_buffer[2048];
    size_t configs_edit_count = N(configs_edit_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, configs_edit_buffer, configs_edit_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        die_with_message("Error recibiendo respuesta para editar configuracion");
    }
    else if (ret != 4)
    {
        die_with_message("Tamaño desconocido de respuesta");
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