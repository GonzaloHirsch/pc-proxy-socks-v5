#include "authentication.h"

static const char * SEP = " ";

bool
validate_up(uint8_t * uid, uint8_t * pw, const char * file_name)
{

    uint8_t * uid_stored;
    uint8_t * pw_stored;
    bool auth_valid = false;

    FILE* file = fopen(file_name, "r");
    if(file == NULL){
        abort();
    }
    
    uint8_t line[256];

    // Simple: Scan every line to see if the user and password matches.
    while (fgets(line, sizeof(line), file) && !auth_valid) {
        
        //Spliting the string to separate user from passwd, get the uid first
        uid_stored = strtok(line, " ");

        // If the user matches
        if(!strcmp(uid, uid_stored)){
            pw_stored = strtok(NULL, "\n");
            //If the password matches
            if(!strcmp(pw, pw_stored)){
                auth_valid = true;
            }
        }   
    }
    fclose(file);

    return auth_valid;
}

bool
validate_user_admin(uint8_t * uid, uint8_t * pw)
{
    validate_up(uid, pw, "./serverdata/admin_user_pass.txt");
}

bool
validate_user_proxy(uint8_t * uid, uint8_t * pw)
{
    validate_up(uid, pw, "./serverdata/user_pass.txt");
}

void list_user_admin(uint8_t ** users, uint8_t * count){
    uint8_t * uid_stored;
    *count = 0;

    // Opening the file to be read
    FILE* file = fopen("./serverdata/admin_user_pass.txt", "r");
    if(file == NULL){
        abort();
    }
    
    uint8_t line[256];

    while (fgets(line, sizeof(line), file)) {
        // Spliting the string to separate user from passwd, get the uid first
        uid_stored = strtok(line, SEP);
        // Copy the username into the array
        users[*count] = malloc(strlen(uid_stored) + 1);
        memcpy(users[*count], uid_stored, strlen(uid_stored) + 1);
        (*count)++;
    }
    fclose(file);
}

bool create_user_admin(uint8_t * username, uint8_t * pass, uint8_t ulen, uint8_t plen){
    // Opening the file to be read
    FILE* file = fopen("./serverdata/admin_user_pass.txt", "a");
    if(file == NULL){
        return false;
    }

    size_t size = 1 + ulen + ulen;
    char * user_pass = malloc(size * sizeof(char));
    sprintf(user_pass, "%s %s", username, pass);

    // Writing to file
    fprintf(file, "\n%s", user_pass);

    fclose(file);
    free(user_pass);

    return true;
}