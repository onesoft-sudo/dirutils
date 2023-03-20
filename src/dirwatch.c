/*
    dirwatch.c -- watch directories for changes.

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
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/inotify.h>

#include "utils.h"

#define EVENT_SIZE (sizeof (struct inotify_event))
#define EVENT_BUF_LEN ((EVENT_SIZE + 16) * 1024)

typedef struct {
    int fd;
    int wd;
    uint32_t mask;
    char *dirpath;
} config_t;

static config_t config;

static struct option const long_options[] = {
    { "events",  required_argument, NULL, 'e' }, /* Experimental. There's no documentation yet and this feature is incomplete. */
    { "help",    no_argument,       NULL, 'h' },
    { "version", no_argument,       NULL, 'v' }
};

static void dirwatch_cleanup()
{
    inotify_rm_watch(config.fd, config.wd);
    close(config.fd);
}

static void sigint_handle()
{
    puts("SIGINT received. Exiting.");
    dirwatch_cleanup();
    exit(EXIT_SUCCESS);
}

static void dirwatch_set_signal_handlers()
{
    struct sigaction action;

    action.sa_handler = sigint_handle;
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) == -1) 
        print_error(true, true, "failed to set SIGINT handler");
}

static void dirwatch_init(char *dirpath)
{
    dirwatch_set_signal_handlers();
    
    config.dirpath = dirpath;
    config.fd = inotify_init();

    if (config.fd == -1)
        print_error(true, true, "cannot initialize inotify");

    config.wd = inotify_add_watch(config.fd, config.dirpath, config.mask);

    if (config.wd == -1)
        print_error(true, true, "%s: cannot watch directory", config.dirpath);
}

static void dirwatch_on_event(struct inotify_event *event)
{
    if (event->len) 
    {
        if (event->mask & IN_CREATE)
            printf("A new %s was created: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_DELETE)
            printf("A %s was deleted: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_ACCESS)
            printf("A %s was accessed: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_MODIFY)
            printf("A %s was modified: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_ATTRIB)
            printf("Attributes of a %s was changed: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_OPEN)
            printf("A %s was opened: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_CLOSE)
            printf("A %s was closed: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_MOVE)
            printf("A %s was moved: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_MOVED_TO)
            printf("A %s was moved to another destination from the current directory: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_MOVED_FROM)
            printf("A %s was moved from another destination to the current directory: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_MOVE_SELF)
            printf("The current %s was moved: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_DELETE_SELF)
            printf("The current %s was deleted: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_CLOSE_WRITE)
            printf("A %s was closed and written to disk: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else if (event->mask & IN_CLOSE_NOWRITE)
            printf("A %s was closed but nothing was written to disk: %s\n", event->mask & IN_ISDIR ? "directory" : "file", event->name);
        else
            printf("Unknown event received: %s\n", event->name);
    }
}

static void dirwatch_watch()
{
    while (true)
    {
        char buffer[EVENT_BUF_LEN];
        int length = read(config.fd, buffer, EVENT_BUF_LEN);

        if (length == -1)
            print_error(true, true, "read from inotify file descriptor failed");

        for (int i = 0; i < length;) 
        {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            dirwatch_on_event(event);
            i += EVENT_SIZE + event->len;
        }
    }
}

static void usage(bool _exit)
{
    fprintf(stdout, "Usage: %s [OPTIONS]... [DIRECTORY]\n\
Watches for changes in DIRECTORY. If no DIRECTORY is specified, it will watch \
the current directory.\n\
\n\
Options:\n\
  -h, --help                   Show this help and exit.\n\
  -v, --version                Show the version of this program.\n\
\n\
This program is a part of dirutils v%s.\n\
", PROGRAM_NAME, VERSION);

    if (_exit)
        exit(EXIT_SUCCESS);
}

static void version(bool _exit)
{
    fprintf(stdout, "%s version %s\n\
Copyright (C) 2023 OSN Inc.\n\
This program is licensed under GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Ar Rakin <rakinar2@onesoftnet.eu.org>.\n", PROGRAM_NAME, VERSION);

    if (_exit)
        exit(EXIT_SUCCESS);
}

static uint32_t dirwatch_parse_event_mask(char *input)
{
    char *delim = ",";
    char *token = strtok(input, delim);
    uint32_t mask = 0;

    while(token != NULL)
    {
        if (STREQ(token, "create") || STREQ(token, "c"))
            mask |= IN_CREATE;
        else if (STREQ(token, "delete") || STREQ(token, "d"))
            mask |= IN_DELETE;
        else if (STREQ(token, "modify") || STREQ(token, "m"))
            mask |= IN_MODIFY;
        else if (STREQ(token, "access") || STREQ(token, "a"))
            mask |= IN_ACCESS;
        else if (STREQ(token, "open") || STREQ(token, "o"))
            mask |= IN_OPEN;
        else if (STREQ(token, "close") || STREQ(token, "l"))
            mask |= IN_CLOSE;
        else if (STREQ(token, "attributes") || STREQ(token, "a"))
            mask |= IN_ATTRIB;
        else if (STREQ(token, "cwrite") || STREQ(token, "w"))
            mask |= IN_CLOSE_WRITE;
        else if (STREQ(token, "ncwrite") || STREQ(token, "f"))
            mask |= IN_CLOSE_NOWRITE;
        else if (STREQ(token, "mvfrom") || STREQ(token, "r"))
            mask |= IN_MOVED_FROM;
        else if (STREQ(token, "move") || STREQ(token, "u"))
            mask |= IN_MOVE;
        else if (STREQ(token, "mvto") || STREQ(token, "t"))
            mask |= IN_MOVED_TO;
        else if (STREQ(token, "delself") || STREQ(token, "s"))
            mask |= IN_DELETE_SELF;
        else if (STREQ(token, "mvself") || STREQ(token, "e"))
            mask |= IN_MOVE_SELF;
        else
            return 0;

        token = strtok(NULL, delim);
    }

    return mask;
}

int main(int argc, char **argv)
{
    set_program_name(argv[0]);

    config.mask = IN_CREATE | IN_DELETE; /* Default mask. */

    while (true)
    {
        int option_index;
        int c = getopt_long(argc, argv, "hve:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                usage(true); /* This function will call exit() itself. */

            case 'v':
                version(true); /* This function will call exit() itself. */

            case 'e':
                {
                    uint32_t mask = dirwatch_parse_event_mask(optarg);

                    if (mask == 0) 
                        print_error(false, true, "invalid events specified\nRun `%s --help' for more detailed information.", PROGRAM_NAME);

                    config.mask = mask;
                }
            break;
 
            case '?':
                fprintf(stderr, "Run `%s --help' for more detailed information.\n", PROGRAM_NAME);
                exit(EXIT_FAILURE);
            break;

            default:
                exit(EXIT_FAILURE);
        }
    }

    char *dirpath = ".";

    for (int i = optind; i < argc; i++)
    {
        dirpath = argv[i];
        break;
    }

    dirwatch_init(dirpath);
    dirwatch_watch();
    dirwatch_cleanup();   

    return 0;
}