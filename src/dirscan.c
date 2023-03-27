/*
    dirscan.c -- scan directories and print the contents.

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

#include <dirent.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "utils.h"
#define MAX_PATHS 128

typedef struct
{
    char **dirpaths;
    size_t count;
    bool recursive;
} config_t;

static struct option const long_options[] = {
    {"help",       no_argument, NULL, 'h'},
    { "recursive", no_argument, NULL, 'r'},
    { "version",   no_argument, NULL, 'v'},
    { NULL,        0,           NULL, 0  }
};

config_t config = { NULL, 0, false };

static void
dirscan_set_dirpath(int argc, char **argv)
{
    for (int i = optind; i < argc; i++)
    {
        if (config.count > MAX_PATHS)
            print_error(false, true, "Too many arguments");

        config.dirpaths
            = xrealloc(config.dirpaths, sizeof(char *) * (config.count + 1));
        config.dirpaths[config.count] = strdup(argv[i]);
        config.count++;
    }
}

static void
dirscan_cleanup()
{
    if (config.dirpaths == NULL)
        return;

    for (int i = 0; i < config.count; i++)
    {
        if (config.dirpaths[i] != NULL)
            free(config.dirpaths[i]);
    }

    free(config.dirpaths);
}

static void
dirscan_init(int argc, char **argv)
{
    if (optind < argc)
        dirscan_set_dirpath(argc, argv);
    else
    {
        config.dirpaths = xmalloc(sizeof(char *));
        config.dirpaths[0] = strdup(".");
        config.count++;
    }
}

static void
dirscan_read_dirent(char *path, u_char type)
{
    if (type == DT_DIR)
    {
        DIR *dir;
        struct dirent *entry;

        puts(path);

        if (!config.recursive)
            return;

        dir = opendir(path);

        if (!dir)
            print_error(true, true, "failed to open child directory: %s",
                        path);

        while ((entry = readdir(dir)) != NULL)
        {
            if (STREQ(entry->d_name, ".") || STREQ(entry->d_name, ".."))
                continue;

            size_t pathlen = strlen(path);
            char *newpath = xmalloc(pathlen + strlen(entry->d_name) + 2);

            strcpy(newpath, path);

            if (path[pathlen - 1] != '/')
                strcat(newpath, "/");

            strcat(newpath, entry->d_name);

            dirscan_read_dirent(newpath, entry->d_type);
            free(newpath);
        }

        closedir(dir);
    }
    else
        puts(path);
}

static void
dirscan_read_dirs()
{
    for (int i = 0; i < config.count; i++)
    {
        DIR *dir = opendir(config.dirpaths[0]);
        struct dirent *entry;

        if (!dir)
            print_error(true, true, "failed to open directory: %s",
                        config.dirpaths[0]);

        while ((entry = readdir(dir)) != NULL)
        {
            if (STREQ(entry->d_name, ".") || STREQ(entry->d_name, ".."))
                continue;

            size_t pathlen = strlen(config.dirpaths[0]);
            char *path = xmalloc(pathlen + strlen(entry->d_name) + 1);

            strcpy(path, config.dirpaths[0]);

            if (path[pathlen - 1] != '/')
                strcat(path, "/");

            strcat(path, entry->d_name);

            dirscan_read_dirent(path, entry->d_type);
            free(path);
        }

        closedir(dir);
    }
}

int
main(int argc, char **argv)
{
    int c, option_index;

    atexit(&dirscan_cleanup);
    set_program_name(argv[0]);

    while ((c = getopt_long(argc, argv, "hrv", long_options, &option_index))
           != -1)
    {
        switch (c)
        {
            case 'h':

                break;

            case 'v':

                break;

            case 'r':
                config.recursive = true;
                break;

            case '?':
            default:
                exit(EXIT_FAILURE);
        }
    }

    dirscan_init(argc, argv);
    dirscan_read_dirs();

    return 0;
}
