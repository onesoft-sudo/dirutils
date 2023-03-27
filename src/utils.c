/*
    utils.c -- common utilities shared in other programs of dirutils.

    Copyright (C) 2023 OSN Inc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "utils.h"
#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *PROGRAM_NAME;

void
set_program_name(char *name)
{
    PROGRAM_NAME = basename(name);
}

void
print_error(bool str_error, bool _exit, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "%s: ", PROGRAM_NAME);
    vfprintf(stderr, fmt, args);

    if (str_error)
    {
        fprintf(stderr, ": %s", strerror(errno));
    }

    fprintf(stderr, "\n");

    va_end(args);

    if (_exit)
        exit(EXIT_FAILURE);
}

void *
xmalloc(size_t size)
{
    void *ptr = malloc(size);

    if (!ptr)
        exit(EXIT_FAILURE);

    return ptr;
}

void *
xrealloc(void *prevptr, size_t size)
{
    void *ptr = realloc(prevptr, size);

    if (!ptr)
        exit(EXIT_FAILURE);

    return ptr;
}
