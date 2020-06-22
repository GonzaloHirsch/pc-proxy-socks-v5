#include "sctpClient_Metrics.h"

static int recv_flags = 0;
static struct sctp_sndrcvinfo sndrcvinfo;

void handle_show_metrics()
{
    // -------------------------------- METRICS LIST REQUEST --------------------------------

    uint8_t metrics_list[] = {0x02, 0x01};

    // Sending login request
    int ret = sctp_sendmsg(serverSocket, (void *)metrics_list, N(metrics_list), NULL, 0, 0, 0, 0, 0, MSG_NOSIGNAL);
    if (ret < 0)
    {
        die_with_message("Error enviando request para mostrar metricas");
    }

    // -------------------------------- METRICS LIST RESPONSE --------------------------------

    uint8_t metrics_list_buffer[2048];
    size_t metrics_list_count = N(metrics_list_buffer);

    // Receiving login response
    ret = sctp_recvmsg(serverSocket, metrics_list_buffer, metrics_list_count, NULL, 0, &sndrcvinfo, &recv_flags);
    if (ret <= 0)
    {
        die_with_message("Error recibiendo respuesta para mostrar metricas");
    }
    else if (ret != 20)
    {
        die_with_message("Tamaño desconocido de respuesta");
    }

    if (metrics_list_buffer[0] != 0x02)
    {
        perror("Tipo diferente al esperado");
        return;
    }

    if (metrics_list_buffer[1] != 0x01)
    {
        perror("Comando diferente al esperado");
        return;
    }

    if (metrics_list_buffer[2] != 0x00)
    {
        perror("Status de error");
        return;
    }

    if (metrics_list_buffer[3] != 0x03)
    {
        perror("Cantidad de metricas inesperada");
        return;
    }

    uint64_t bytes = ntoh64(metrics_list_buffer + 4);
    uint32_t hist = ntoh32(metrics_list_buffer + 4 + 8);
    uint32_t curr = ntoh32(metrics_list_buffer + 4 + 8 + 4);

    printf("Cantidad de bytes transferidos: %lu\n", bytes);
    printf("Conexiones históricas: %u\n", hist);
    printf("Conexiones concurrentes: %u", curr);
}