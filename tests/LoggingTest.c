#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    init_requests_log();
    init_credentials_log();
    char *origin_ip = "192.168.1.1";
    char *dest_ip = "234.53.35.32";
    char *origin_port = "1234";
    char *dest_port = "80";
    log_request(origin_ip, origin_port, dest_ip, dest_port);
    log_request(dest_ip, dest_port, origin_ip, origin_port);
    log_credential("gonza", "password123");
    log_credential("hola", "passwed");
    close_requests_log();
    close_credentials_log();
    return 0;
}