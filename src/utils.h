/*
    utils.h -- typedefs and prototypes for the common shared utilities.

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

#ifndef __UTILS_H__
#define __UTILS_H__

#include "config.h"
#include <stdbool.h>

#ifndef VERSION
#define VERSION "0.0.1"
#endif

typedef enum
{
    VERBOSITY_1 = 1,
    VERBOSITY_2,
    VERBOSITY_3
} verbosity_t;

#ifdef USE_COLORS
#define COLOR(c, s) "\033[" c "m" s "\033[0m"
#else
#define COLOR(c, s) s
#endif

#ifndef _NDEBUG_1
#define LOG_DEBUG_1(l, s, ...)                                                \
    if (l >= VERBOSITY_1)                                                     \
    printf("DEBUG(1): " s, __VA_ARGS__)
#else
#define LOG_DEBUG_1(l, s, ...) NULL
#endif

#ifndef _NDEBUG_2
#define LOG_DEBUG_2(l, s, ...)                                                \
    if (l >= VERBOSITY_2)                                                     \
    printf("DEBUG(2): " s, __VA_ARGS__)
#else
#define LOG_DEBUG_2(l, s, ...) NULL
#endif

#ifndef _NDEBUG_3
#define LOG_DEBUG_3(l, s, ...)                                                \
    if (l == VERBOSITY_3)                                                     \
    printf("DEBUG(3): " s, __VA_ARGS__)
#else
#define LOG_DEBUG_3(l, s, ...) NULL
#endif

#define STREQ(s1, s2) strcmp(s1, s2) == 0

extern char *PROGRAM_NAME;

void set_program_name(char *name);
void print_error(bool str_error, bool _exit, const char *fmt, ...);

#endif
