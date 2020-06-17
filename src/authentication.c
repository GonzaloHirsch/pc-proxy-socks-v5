#include "authentication.h"

static struct user_pass user_pass_table[MAX_USER_PASS];
static int user_pass_c;

int create_table(char *filename, auth_level level)
{
    uint8_t line[514], *uid_stored, *pw_stored, *uid, *pw;

    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        return -1;
    }
    while (fgets((char *)line, sizeof(line), file) && user_pass_c < MAX_USER_PASS)
    {

        //Spliting the string to separate user from passwd, get the uid first
        uid_stored = (uint8_t *)strtok((char *)line, " ");
        pw_stored = (uint8_t *)strtok(NULL, "\n");

        uid = malloc(1 + strlen((const char *)uid_stored));
        if (uid == NULL)
            return -1;

        pw = malloc(1 + strlen((const char *)pw_stored));
        if (pw == NULL)
            return -1;

        strcpy((char *)uid, (const char *)uid_stored);
        strcpy((char *)pw, (const char *)pw_stored);

        user_pass_table[user_pass_c].user = uid;
        user_pass_table[user_pass_c].password = pw;
        user_pass_table[user_pass_c].level = level;
        user_pass_c++;
    }
    fclose(file);

    return 0;
}

int create_up_table()
{
    return create_table("./serverdata/user_pass.txt", USER_AUTH_LEVEL);
}

int create_up_admin_table()
{
    return create_table("./serverdata/admin_user_pass.txt", ADMIN_AUTH_LEVEL);
}

bool validate_up(uint8_t *uid, uint8_t *pw, uint8_t level)
{
    uint8_t *uid_stored;
    uint8_t *pw_stored;
    bool auth_valid = false;
    int i = 0;

    // Simple: Scan every user and password to see if the user and password matches.
    while (i < user_pass_c && !auth_valid)
    {
        if (user_pass_table[i].level == level)
        {
            //Spliting the string to separate user from passwd, get the uid first
            uid_stored = user_pass_table[i].user;

            // If the user matches
            if (!strcmp((const char *)uid, (const char *)uid_stored))
            {
                pw_stored = user_pass_table[i].password;
                //If the password matches
                if (!strcmp((const char *)pw, (const char *)pw_stored))
                {
                    auth_valid = true;
                }
            }
        }
        i++;
    }

    return auth_valid;
}

bool validate_user_admin(uint8_t *uid, uint8_t *pw)
{
    return validate_up(uid, pw, ADMIN_AUTH_LEVEL);
}

bool validate_user_proxy(uint8_t *uid, uint8_t *pw)
{
    return validate_up(uid, pw, USER_AUTH_LEVEL);
}

void list_user_admin(uint8_t **users, uint8_t *count)
{
    uint8_t *uid_stored;
    *count = 0;
    int i = 0;

    while (i < user_pass_c)
    {
        if (user_pass_table[i].level == ADMIN_AUTH_LEVEL)
        {
            // Getting the username + password
            uid_stored = user_pass_table[i].user;
            users[*count] = malloc(strlen(uid_stored) + 1);
            memcpy(users[*count], uid_stored, strlen(uid_stored) + 1);
            (*count)++;
        }
        i++;
    }
}

bool create_user(uint8_t *username, uint8_t *pass, auth_level level)
{
    uint8_t *uid, *pw;

    uid = malloc(1 + strlen((const char *)username));
    if (uid == NULL)
        return false;

    pw = malloc(1 + strlen((const char *)pass));
    if (pw == NULL)
        return false;

    strcpy((char *)uid, (const char *)username);
    strcpy((char *)pw, (const char *)pass);

    user_pass_table[user_pass_c].user = uid;
    user_pass_table[user_pass_c].password = pw;
    user_pass_table[user_pass_c].level = level;
    user_pass_c++;

    return true;
}

bool create_user_proxy(uint8_t *username, uint8_t *pass)
{
    return create_user(username, pass, USER_AUTH_LEVEL);
}

bool create_user_admin(uint8_t *username, uint8_t *pass)
{
    return create_user(username, pass, ADMIN_AUTH_LEVEL);
}