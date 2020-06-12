#include "../include/logging.h"

static FILE * requests_log_ptr;
static FILE * credentials_log_ptr;

void init_requests_log(){
    // Opening the file
    requests_log_ptr = fopen(REQUESTS_FILE, "a");
    if (requests_log_ptr == NULL){
        // HANDLE ERROR
        printf("Error initializing the requests file");
    }
}

void init_credentials_log(){
    // Opening the file
    credentials_log_ptr = fopen(CREDENTIALS_FILE, "a");
    if (credentials_log_ptr == NULL){
        // HANDLE ERROR
        printf("Error initializing the credentials file");
    }
}

void log_request(uint8_t * client_address, uint8_t * client_port, uint8_t * origin_address, uint8_t * origin_port){
    if (requests_log_ptr != NULL){
        // Getting the time of now
        time_t now = 0;
        time(&now);

        // Generaing a human readable time
        uint8_t time_buff[100];
        strftime(time_buff, N(time_buff), "%x %X", localtime(&now));

        // Generating the string to output on the log line
        size_t size = " - : -> :\n" + strlen(time_buff) + strlen(client_address) + strlen(client_port) + strlen(origin_address) + strlen(origin_port);
        char * log = malloc(size * sizeof(char));
        sprintf(log, "%s - %s:%s -> %s:%s\n", time_buff, client_address, client_port, origin_address, origin_port);

        // Writing to file
        fprintf(requests_log_ptr, "%s", log);

        // Freeing the pointer to the
        free(log);
    }
}

void log_credential(uint8_t * user, uint8_t * pass){
    if (credentials_log_ptr != NULL){
        // Getting the time of now
        time_t now = 0;
        time(&now);

        // Generaing a human readable time
        uint8_t time_buff[100];
        strftime(time_buff, N(time_buff), "%x %X", localtime(&now));

        // Generating the string to output on the log line
        size_t size = " - :\n" + strlen(time_buff) + strlen(user) + strlen(pass);
        char * log = malloc(size * sizeof(char));
        sprintf(log, "%s - %s:%s\n", time_buff, user, pass);

        // Writing to file
        fprintf(credentials_log_ptr, "%s", log);

        // Freeing the pointer to the
        free(log);
    }
}

void close_requests_log(){
    if (requests_log_ptr != NULL){
        fclose(requests_log_ptr);
    }
}

void close_credentials_log(){
    if (credentials_log_ptr != NULL){
        fclose(credentials_log_ptr);
    }
}