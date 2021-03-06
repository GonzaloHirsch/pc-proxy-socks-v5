#include "sctpClient_Configs.h"

static int recv_flags = 0;
static struct sctp_sndrcvinfo sndrcvinfo;

void send_edit_config(Configs conf, uint8_t *data, ssize_t data_len);
int show_config_options();

bool get_16_bit_number(uint16_t *n)
{
    printf("Nuevo valor para configuración (1 - 65535): ");

    char option[255] = {0};
    char * res = fgets(option, 255, stdin);

    if (res == NULL || option == NULL)
    {
        return false;
    } 

    int i = atoi(option);

    if (i == 0 || (i > 65535 || i <= 0))
    {
        return false;
    }

    *n = i;

    return true;
}

bool get_8_bit_number(uint8_t *n)
{
    printf("Nuevo valor para configuración (0 o 1): ");

    char option[255] = {0};
    char * res = fgets(option, 255, stdin);

    if (res == NULL || option == NULL)
    {
        return false;
    } 

    int i = atoi(option);

    if (i != 0 && i != 1){
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
    else if (ret != 9)
    {
        die_with_message("Tamaño desconocido de respuesta");
    }

    if (configs_list_buffer[0] != 0x03)
    {
        printf("Error: Tipo diferente al esperado");
        return;
    }

    if (configs_list_buffer[1] != 0x01)
    {
        printf("Error: Comando diferente al esperado");
        return;
    }

    if (configs_list_buffer[2] != 0x00)
    {
        determine_error(configs_list_buffer[2]);
        return;
    }

    if (configs_list_buffer[3] != 0x03)
    {
        printf("Error: Cantidad de configuraciones inesperada");
        return;
    }

    uint16_t socks5_buff_len = ntoh16(configs_list_buffer + 4);
    uint16_t sctp_buff_len = ntoh16(configs_list_buffer + 6);
    uint8_t disectors = configs_list_buffer[8];

    printf("Tamaño del buffer del proxy socks5 (bytes): %d\n", socks5_buff_len);
    printf("Tamaño del buffer del socket de management (bytes): %d\n", sctp_buff_len);
    printf("Estado de los disectores: %s", disectors > 0 ? "Activos" : "Inactivos");
}

int show_config_options()
{
    printf("Posibles configuraciones para editar:\n"
           "1 - Tamaño de buffer de proxy\n"
           "2 - Tamaño de buffer de management\n"
           "3 - Estado de los disectores\n"
           "\t0 -> Desactivar\n"
           "\t1 -> Activar\n"
           "100 - Volver para Atrás\n");

    printf("Elegir un número de configuración: ");
    char option[255] = {0};
    char * res = fgets(option, 255, stdin);

    if (res == NULL || option == NULL)
    {
        return -1;
    } 

    int i = atoi(option);

    if (i == 0 || ((i <= 0 || i > 3) && (i != 100))){
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
        case CONF_SCTP_BUFF:
            printf("Opción %d - Tamaño de Buffer de SCTP\n\n", option);
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
            printf("Opción %d - Tamaño de Buffer de Proxy\n\n", option);
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
        case CONF_DISECTORS:
            printf("Opción %d - Estado de los disectores\n\n", option);
            parseOk = get_8_bit_number(&val.val8);
            if (!parseOk || (val.val8 != 1 && val.val8 != 0))
            {
                handle_invalid_value();
            }
            else
            {
                data.data8[3] = val.val8 == 0 ? 0 : 1;
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
        printf("Error: Tipo diferente al esperado");
        return;
    }

    if (configs_edit_buffer[1] != 0x03)
    {
        printf("Error: Comando diferente al esperado");
        return;
    }

    if (configs_edit_buffer[2] != 0x00)
    {
        determine_error(configs_edit_buffer[2]);
        return;
    }

    if (configs_edit_buffer[3] != conf)
    {
        printf("Configuración inesperada");
        return;
    }

    printf("Configuración actualizada con éxito");
}