#include "authentication.h"

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