#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>  
#include <stdarg.h>
#include "utils.h"

char *PROGRAM_NAME;

void set_program_name(char *name) 
{
    PROGRAM_NAME = name;
}

void print_error(bool str_error, const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);

    printf("%s: ", PROGRAM_NAME);
    vprintf(fmt, args);

    if (str_error) {
        printf(": %s", strerror(errno));
    }

    printf("\n");

    va_end(args);
    exit(EXIT_FAILURE);
}

char *strcat_malloc(char *s1, char *s2, ...) 
{
    size_t s1_len = strlen(s1);
    size_t s2_len = strlen(s2);
    size_t ptrsize = s1_len + s2_len;

    char *ptr = malloc(ptrsize + 1);

    int i;

    if (ptr == NULL) 
        return NULL;
    
    for (i = 0; i < s1_len; i++)
        ptr[i] = s1[i];

    for (int j = 0; i < ptrsize && j < s2_len; i++, j++)
        ptr[i] = s2[j];

    va_list args;
    va_start(args, s2);
    char *tmp_s = NULL;

    while ((tmp_s = va_arg(args, char *)) != NULL) 
    {
        size_t len = strlen(tmp_s);
        size_t prevsize = ptrsize;

        ptrsize += len;
        ptr = realloc(ptr, ptrsize);

        for (int i = 0; i < strlen(tmp_s); i++)
            ptr[prevsize + i] = tmp_s[i];
    }

    va_end(args);

    ptr[ptrsize] = '\0';

    return ptr;
}
