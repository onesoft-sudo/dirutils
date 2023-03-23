/*
    dirmap.h -- typedefs and prototypes for dirmap.c

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

#ifndef __DIRMAP_H__
#define __DIRMAP_H__

#include <stddef.h>
#include <stdbool.h>

#define DIRMAP_INIT { .entries = NULL, .size = 0 }

typedef struct {
    char *dirpath;
    int wd;
} dirmap_entry_t;

typedef struct {
    dirmap_entry_t *entries;
    size_t size;
    size_t max_dirpath_len;
} dirmap_t;

__BEGIN_DECLS

void dirmap_init(dirmap_t *map);
bool dirmap_add(dirmap_t *map, char *dirpath, int wd);
void dirmap_free(dirmap_t *map);
dirmap_entry_t *dirmap_find_by_wd(dirmap_t *map, int wd);

__END_DECLS

#endif
