#include "authentication.h"

static struct user_pass user_pass_table[MAX_USER_PASS];
static int user_pass_c;

int create_up_table(){

    uint8_t line[514], * uid_stored, * pw_stored, * uid, * pw;

    FILE* file = fopen("./serverdata/user_pass.txt", "r");
    if(file == NULL){
        return -1;
    }
    while (fgets((char *) line, sizeof(line), file) && user_pass_c < MAX_USER_PASS) {
        
        //Spliting the string to separate user from passwd, get the uid first
        uid_stored = (uint8_t *) strtok((char *) line, " ");
        pw_stored = (uint8_t*) strtok(NULL, "\n");

        uid = malloc(1 + strlen((const char *)uid_stored));
        if(uid == NULL)
            return -1;
        
        pw = malloc(1 + strlen((const char *)pw_stored));
        if(pw == NULL)
            return -1;

        strcpy((char *)uid,(const char *)uid_stored);
        strcpy((char *)pw, (const char *)pw_stored);

        user_pass_table[user_pass_c].user = uid;
        user_pass_table[user_pass_c].password = pw;
        user_pass_c++;
          
    }
    fclose(file);

    return 0;
}


bool
validate_up(uint8_t * uid, uint8_t * pw, uint8_t level)
{
    uint8_t * uid_stored;
    uint8_t * pw_stored;
    bool auth_valid = false;
    int i = 0;

    // Simple: Scan every user and password to see if the user and password matches.
    while (i < user_pass_c && !auth_valid) {
        
        //Spliting the string to separate user from passwd, get the uid first
        uid_stored = user_pass_table[i].user;

        // If the user matches
        if(!strcmp((const char *) uid, (const char *) uid_stored)){
            pw_stored = user_pass_table[i].password;
            //If the password matches
            if(!strcmp((const char *) pw, (const char *) pw_stored)){
                auth_valid = true;
            }
        }
        i++;   
    }

    return auth_valid;
}

bool
validate_user_admin(uint8_t * uid, uint8_t * pw)
{
    return validate_up(uid, pw, USER_AUTH_LEVEL);
}

bool
validate_user_proxy(uint8_t * uid, uint8_t * pw)
{
    return validate_up(uid, pw, ADMIN_AUTH_LEVEL);
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
    FILE* file = fopen("./serverdata/admin_user_pass.txt", "a+");
    if(file == NULL){
        return false;
    }

    uint8_t * uid_stored;
    uint8_t line[256];

    // Checking if the username already exists
    while (fgets(line, sizeof(line), file)) {
        // Spliting the string to separate user from passwd, get the uid first
        uid_stored = strtok(line, SEP);
        if (strcmp(uid_stored, username) == 0){
            return false;
        }
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