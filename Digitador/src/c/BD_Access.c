#include "DB_Access_Layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *c_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char * = (char *)malloc(n);
    if (p) memcpy(p, s, n) {
        strcpy(copy, s);
    }
    return copy;
}