/*
    dirmap.c -- map directories with their watch descriptors.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "dirmap.h"

void dirmap_init(dirmap_t *map)
{
    map->entries = NULL;
    map->size = 0;
    map->max_dirpath_len = 0;
}

bool dirmap_add(dirmap_t *map, char *dirpath, int wd)
{
    assert(dirpath);
    assert(wd > 0);

    map->entries = realloc(map->entries, sizeof (dirmap_entry_t) * (map->size + 1));

    if (map->entries == NULL)
    {
        return false;
    }

    map->entries[map->size++] = (dirmap_entry_t) {
        dirpath: strdup(dirpath),
        wd: wd
    };

    size_t len = strlen(dirpath);

    map->max_dirpath_len = map->max_dirpath_len < len ? len : map->max_dirpath_len; 
    
    return true;
}

dirmap_entry_t *dirmap_find_by_wd(dirmap_t *map, int wd)
{
    assert(map);
    assert(wd > 0);

    for (size_t i = 0; i < map->size; i++)
    {
        if (map->entries[i].wd == wd)
            return &map->entries[i];
    }

    return NULL;
}

void dirmap_free(dirmap_t *map)
{
    if (map->entries != NULL)
    {
        for (size_t i = 0; i < map->size; i++)
            if (map->entries[i].dirpath != NULL)
                free(map->entries[i].dirpath);

        free(map->entries);
        map->entries = NULL;
    }
}
