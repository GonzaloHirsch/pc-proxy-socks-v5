#include "authentication.h"

static struct user_pass user_pass_table[MAX_USER_PASS];
static int user_pass_c;

int create_table(char *filename, auth_level level)
{
    uint8_t line[514] = {0x00}, *uid_stored, *pw_stored, *uid, *pw;
    int ulen, plen;

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

        ulen = strlen((const char *)uid_stored);
        plen = strlen((const char *)pw_stored);

        uid = calloc(1 + ulen, sizeof(uint8_t));
        if (uid == NULL)
            return -1;

        pw = calloc(1 + plen, sizeof(uint8_t));
        if (pw == NULL)
            return -1;

        memset(uid, 0, 1 + ulen);
        memset(pw, 0, 1 + plen);
        memcpy(uid, uid_stored, ulen);
        memcpy(pw, pw_stored, plen);
        uid[ulen] = 0x00;
        pw[plen] = 0x00;

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

void list_user_admin(uint8_t **users, uint8_t *count, int *size)
{
    uint8_t *uid_stored;
    *count = 0;
    int i = 0, slen = 0;

    while (i < user_pass_c)
    {
        if (user_pass_table[i].level == ADMIN_AUTH_LEVEL)
        {
            // Getting the username with the finishing
            uid_stored = user_pass_table[i].user;
            slen = strlen((const char *)uid_stored);
            users[*count] = calloc(slen + 1, sizeof(uint8_t));
            memcpy(users[*count], uid_stored, slen);
            users[*count][slen] = 0x00;
            (*size) += slen;
            (*count)++;
        }
        i++;
    }
}

void free_list_user_admin(uint8_t **users, uint8_t count)
{
    int i;
    for (i = 0; i < count; i++)
    {
        if (i % 3 == 0)
        {
        }
        else if (i % 3 == 1)
        {
            free(users[i]);
        }
        if (i % 3 == 2)
        {
        }
    }
}

bool create_user(uint8_t *username, uint8_t *pass, uint8_t ulen, uint8_t plen, auth_level level, bool canOverride)
{
    uint8_t *uid, *pw;
    int i = 0;
    bool exists = false;

    while (i < user_pass_c && !exists)
    {
        if (user_pass_table[i].level == level)
        {
            // The user already exists
            if (strcmp((const char *)user_pass_table[i].user, (const char *)username) == 0)
            {
                exists = true;
                // If the user can be overrided with a new password, set the new password
                if (!canOverride)
                {
                    return false;
                }

                // Reallocating the memory to fit the new password
                user_pass_table[i].password = realloc(user_pass_table[i].password, 1 + plen);
                if (user_pass_table[i].password == NULL)
                {
                    return false;
                }

                // Putting the new password
                memcpy(user_pass_table[i].password, pass, plen);
                user_pass_table[i].password[plen] = 0x00;
                return true;
            }
        }
        i++;
    }

    if (!exists)
    {
        uid = malloc(1 + ulen);
        if (uid == NULL)
            return false;

        pw = malloc(1 + plen);
        if (pw == NULL)
            return false;

        memcpy((char *)uid, username, ulen);
        uid[ulen] = 0x00;
        memcpy((char *)pw, pass, plen);
        pw[plen] = 0x00;

        user_pass_table[user_pass_c].user = uid;
        user_pass_table[user_pass_c].password = pw;
        user_pass_table[user_pass_c].level = level;
        user_pass_c++;
    }

    return true;
}

bool create_user_proxy(uint8_t *username, uint8_t *pass, uint8_t ulen, uint8_t plen)
{
    return create_user(username, pass, ulen, plen, USER_AUTH_LEVEL, true);
}

bool create_user_admin(uint8_t *username, uint8_t *pass, uint8_t ulen, uint8_t plen)
{
    return create_user(username, pass, ulen, plen, ADMIN_AUTH_LEVEL, false);
}