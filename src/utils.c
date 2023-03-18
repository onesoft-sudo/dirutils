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

void print_error(bool str_error, bool _exit, const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "%s: ", PROGRAM_NAME);
    vfprintf(stderr, fmt, args);

    if (str_error) {
        fprintf(stderr, ": %s", strerror(errno));
    }

    fprintf(stderr, "\n");

    va_end(args);

    if (_exit)
        exit(EXIT_FAILURE);
}
