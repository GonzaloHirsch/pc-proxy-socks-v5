#include "io_utils/strings.h"

#include <string.h>
#include <ctype.h>

int i_strcmp(const char * a, const char * b) {
    const char * p1 = a;
    const char * p2 = b;
    for (; *p1 && *p2; p1++, p2++) {
        if (tolower(*p1) != tolower(*p2))
            return tolower(*p1) - tolower(*p2);
    }
    return tolower(*p1) - tolower(*p2);
}