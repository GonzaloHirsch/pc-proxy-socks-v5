#include "logging.h"

void log_request(const int status, const struct sockaddr *clientaddr, const struct sockaddr *originaddr)
{
    char cbuff[SOCKADDR_TO_HUMAN_MIN * 2 + 2 + 32] = {0};
    unsigned n = N(cbuff);
    time_t now = 0;
    time(&now);

    // tendriamos que usar gmtime_r pero no está disponible en C99
    strftime(cbuff, n, "%FT%TZ\t", gmtime(&now));
    size_t len = strlen(cbuff);
    sockaddr_to_human(cbuff + len, N(cbuff) - len, clientaddr);
    strncat(cbuff, "\t", n - 1);
    cbuff[n - 1] = 0;
    len = strlen(cbuff);
    sockaddr_to_human(cbuff + len, N(cbuff) - len, originaddr);

    fprintf(stdout, "%s\tstatus=%d\n", cbuff, status);
}