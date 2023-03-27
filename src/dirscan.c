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
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"
#define MAX_PATHS 128

typedef struct
{
    char **dirpaths;
    size_t count;
    bool recursive;
    FILE *outbuf;
} config_t;

static struct option const long_options[] = {
    {"help",       no_argument,       NULL, 'h'},
    { "recursive", no_argument,       NULL, 'r'},
    { "version",   no_argument,       NULL, 'v'},
    { "output",    required_argument, NULL, 'o'},
    { NULL,        0,                 NULL, 0  }
};

config_t config = { NULL, 0, false, NULL };

static void
outbuf_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(config.outbuf, fmt, args);
    va_end(args);
}

static int
outbuf_puts(char *s)
{
    return fprintf(config.outbuf, "%s\n", s);
}

__attribute__((__nonnull__)) static void
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
    if (fileno(config.outbuf) != fileno(stdout))
    {
        fclose(config.outbuf);
    }

    if (config.dirpaths == NULL)
        return;

    for (int i = 0; i < config.count; i++)
    {
        if (config.dirpaths[i] != NULL)
            free(config.dirpaths[i]);
    }

    free(config.dirpaths);
}

__attribute__((__nonnull__)) static void
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

__attribute__((__nonnull__)) static void
dirscan_read_dirent(char *path, u_char type)
{
    if (type == DT_DIR)
    {
        DIR *dir;
        struct dirent *entry;

        outbuf_printf("%s/\n", path);

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
        outbuf_puts(path);
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
            char *path = xmalloc(pathlen + strlen(entry->d_name) + 2);

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

static void
usage(bool _exit)
{
    printf("Usage: %s [OPTION]... [DIRECTORY]...\n\
Scans the given DIRECTORY or DIRECTORIES and prints the file paths in the\
 DIRECTORY or DIRECTORIES.\n\
\n\
Options:\n\
  -h, --help              Show this help and exit.\n\
  -o, --output=<FILE>     Save the scanned file list into the FILE.\n\
  -r, --recursive         Scan the directories recursively.\n\
  -v, --version           Show the version information of this program.\n\
\n\
This program is a part of dirutils v%s.\n\
Report bugs to: <%s>.\n\
Dirutils home page: <%s>.\n\
",
           PROGRAM_NAME, VERSION, PACKAGE_BUGREPORT, PACKAGE_URL);

    if (_exit)
        exit(EXIT_SUCCESS);
}

static void
version(bool _exit)
{
    printf("%s (dirutils) version %s\n\
Copyright (C) 2023 OSN Inc.\n\
This program is licensed under GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Ar Rakin <rakinar2@onesoftnet.eu.org>.\n",
           PROGRAM_NAME, VERSION);

    if (_exit)
        exit(EXIT_SUCCESS);
}

int
main(int argc, char **argv)
{
    int c, option_index;

    config.outbuf = stdout;

    atexit(&dirscan_cleanup);
    set_program_name(argv[0]);

    while ((c = getopt_long(argc, argv, "hrvo:", long_options, &option_index))
           != -1)
    {
        switch (c)
        {
            case 'h':
                usage(true);
                break;

            case 'v':
                version(true);
                break;

            case 'r':
                config.recursive = true;
                break;

            case 'o':
            {
                if (access(optarg, F_OK) == 0)
                {
                    char buf[128];

                    buf[1] = '\0';

                    printf("This will overwrite the existing file (%s), do "
                           "you want to continue? [Y/n] ",
                           optarg);

                    fgets(buf, sizeof buf, stdin);

                    if ((buf[1] != '\n' && buf[2] != '\0')
                        || (buf[0] != 'y' && buf[0] != 'Y'))
                    {
                        printf("Operation cancelled.\n");
                        exit(EXIT_SUCCESS);
                    }
                }

                FILE *fp = fopen(optarg, "w");

                if (!fp)
                    print_error(true, true, "Could not open file: %s", optarg);

                config.outbuf = fp;
            }
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
