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

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

#include "dirmap.h"
#include "utils.h"

#define INOTIFY_MAX_USER_WATCHES_FILE "/proc/sys/fs/inotify/max_user_watches"
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN ((EVENT_SIZE + 16) * 1024)
#define IN_DEFAULT (IN_CREATE | IN_MOVE | IN_DELETE | IN_MODIFY | IN_ATTRIB)

typedef uint32_t mask_t; /* inotify mask type. */

/* Configuration of the program.
   TODO: Add support of recursively watching directories. */
typedef struct
{
    int fd;          /* The file descriptor provided by inotify_init(). */
    int wd;          /* Watch descriptor for the given directory. */
    mask_t mask;     /* Watch descriptor for the given directory. */
    char *dirpath;   /* Path of the directory to watch. */
    int watchcount;  /* Count of the watchers in total. */
    int max_watches; /* Max count of the watchers in total. */
    bool recursive;  /* Flag set by options. */
    verbosity_t verbosity; /* Verbosity level set by options. */
} config_t;

/* Event color and stringified representation. */
typedef struct
{
    char *eventstr; /* The stringified representation of the event. */
    int colorcode;  /* Appropriate color code for the event. */
} event_info_t;

/* The main configuration variable for the whole program. */
static config_t config;

/* Directory list map. */
static dirmap_t dirmap = DIRMAP_INIT;

/* Command-line options. */
static struct option const long_options[] = {
    {"events",     required_argument, NULL, 'e'},
    { "help",      no_argument,       NULL, 'h'},
    { "recursive", no_argument,       NULL, 'r'},
    { "verbose",   optional_argument, NULL, 'V'},
    { "version",   no_argument,       NULL, 'v'}
};

/* Close the file and watch descriptors. */
static void
dirwatch_cleanup()
{
    if (config.wd)
        inotify_rm_watch(config.fd, config.wd);

    dirmap_free(&dirmap);
    close(config.fd);
}

/* In case if we receive a SIGINT signal, then print the text into STDOUT and
   exit. */
static void
sigint_handle()
{
    puts("SIGINT received. Exiting.");
    dirwatch_cleanup();
    exit(EXIT_SUCCESS);
}

/* Sets the signal handler functions. */
static void
dirwatch_set_signal_handlers()
{
    struct sigaction action;

    action.sa_handler = sigint_handle;
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) == -1)
        print_error(true, true, "failed to set SIGINT handler");
}

static int
dirwatch_get_max_watches()
{
    FILE *fp = fopen(INOTIFY_MAX_USER_WATCHES_FILE, "r");

    if (fp == NULL)
        return -1;

    int max_watches;

    (void) fscanf(fp, "%d", &max_watches);
    fclose(fp);

    return max_watches;
}

static bool
dirwatch_add_watches_recursive(char *dirpath)
{
    assert(dirpath);

    DIR *dir = opendir(dirpath);

    if (dir == NULL)
        return false;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (config.watchcount >= config.max_watches)
        {
            closedir(dir);
            errno = ENOBUFS; /* Set error in case if the max limit was
                                reached. */
            return false;
        }

        if (STREQ(entry->d_name, ".") || STREQ(entry->d_name, ".."))
            continue;

        if (entry->d_name[0] == '.')
            continue;

        if (entry->d_type == DT_DIR)
        {
            size_t len = strlen(dirpath) + strlen(entry->d_name) + 1;
            char *newpath = malloc(len);

            if (newpath == NULL)
            {
                closedir(dir);
                return false;
            }

            strcpy(newpath, dirpath);
            strcat(newpath, "/");
            strcat(newpath, entry->d_name);

            LOG_DEBUG_2(config.verbosity,
                        "Attempting to watch directory: %s\n", newpath);

            int wd = inotify_add_watch(config.fd, newpath, config.mask);

            if (wd == -1)
            {
                LOG_DEBUG_1(config.verbosity,
                            "Failed to watch directory: %s\n", newpath);
                free(newpath);
                closedir(dir);
                return false;
            }

            if (!dirmap_add(&dirmap, newpath, wd))
            {
                LOG_DEBUG_1(config.verbosity,
                            "Failed to add watched directory to map: %s\n",
                            newpath);
                free(newpath);
                closedir(dir);
                return false;
            }

            LOG_DEBUG_1(config.verbosity, "Watching directory: %s\n", newpath);

            if (!dirwatch_add_watches_recursive(newpath))
            {
                LOG_DEBUG_1(config.verbosity, "Recursive watch failed: %s\n",
                            newpath);
                free(newpath);
                closedir(dir);
                return false;
            }

            free(newpath);
            config.watchcount++;
        }
    }

    closedir(dir);
    return true;
}

/* Initializes the program and its resources. */
static void
dirwatch_init(char *dirpath)
{
    dirwatch_set_signal_handlers();

    config.watchcount = 0;
    config.dirpath = dirpath;

    int max = dirwatch_get_max_watches();

    if (max == -1)
        print_error(true, true, "failed to get max watch count");

    config.max_watches = max;
    config.fd = inotify_init();

    if (config.fd == -1)
        print_error(true, true, "cannot initialize inotify");

    LOG_DEBUG_2(config.verbosity, "Attempting to watch directory: %s\n",
                config.dirpath);

    /* Watch for changes in this directory. Only notify for the given events
        in the mask parameter. */
    config.wd = inotify_add_watch(config.fd, config.dirpath, config.mask);

    if (config.wd == -1)
        print_error(true, true, "%s: cannot watch directory", config.dirpath);

    LOG_DEBUG_1(config.verbosity, "Watching directory: %s\n", config.dirpath);

    if (config.recursive)
    {
        if (!dirmap_add(&dirmap, config.dirpath, config.wd))
            print_error(true, true, "%s: cannot add watched directory to map",
                        config.dirpath);

        if (!dirwatch_add_watches_recursive(config.dirpath))
            print_error(true, true, "%s: cannot recursively watch directory",
                        config.dirpath);
    }

    atexit(&dirwatch_cleanup);
}

/* Cleans up the dynamically allocated string returned by
   dirwatch_event_info(). */
static bool
dirwatch_free_event_info(event_info_t *info)
{
    if (info->eventstr != NULL)
        free(info->eventstr);
}

/* Determine the event string representation for outputting into STDOUT.
   The string is dynamically allocated using strdup(), so it should be
   freed when done working with it. */
static bool
dirwatch_event_info(event_info_t *info, mask_t mask)
{
    char *string = NULL;
    size_t len = 10;

    if (mask & IN_CREATE)
    {
        string = "CREATE    ";
        info->colorcode = 32;
    }
    else if (mask & IN_DELETE)
    {
        string = "DELETE    ";
        info->colorcode = 31;
    }
    else if (mask & IN_ACCESS)
    {
        string = "READ      ";
        info->colorcode = 34;
    }
    else if (mask & IN_MODIFY)
    {
        string = "MODIFIED  ";
        info->colorcode = 33;
    }
    else if (mask & IN_ATTRIB)
    {
        string = "ATTRCHANGE";
        info->colorcode = 33;
    }
    else if (mask & IN_OPEN)
    {
        string = "OPENED    ";
        info->colorcode = 34;
    }
    else if (mask & IN_CLOSE)
    {
        string = "CLOSED    ";
        info->colorcode = 34;
    }
    else if (mask & IN_MOVED_TO)
    {
        string = "MOVEDTO   ";
        info->colorcode = 33;
    }
    else if (mask & IN_MOVED_FROM)
    {
        string = "MOVEDFROM ";
        info->colorcode = 33;
    }
    else if (mask & IN_MOVE_SELF)
    {
        string = "MOVEDSELF ";
        info->colorcode = 33;
    }
    else if (mask & IN_MOVE)
    {
        string = "MOVED     ";
        info->colorcode = 33;
    }
    else if (mask & IN_DELETE_SELF)
    {
        string = "DELSELF   ";
        info->colorcode = 31;
    }
    else if (mask & IN_CLOSE_WRITE)
    {
        string = "CWRITE    ";
        info->colorcode = 32;
    }
    else if (mask & IN_CLOSE_NOWRITE)
    {
        string = "NCWRITE   ";
        info->colorcode = 31;
    }
    else
        return false;

    assert(strlen(string) == len);
    info->eventstr = strdup(string);

    return true;
}

/* Log the given event into STDOUT. */
static bool
dirwatch_log_event(mask_t mask, char *name, char *context_dir)
{
    assert(name);

    event_info_t info = { 0 };

    if (!dirwatch_event_info(&info, mask))
        return false;

    fprintf(stdout, COLOR("1;%d", "%s") " %s%s", info.colorcode, info.eventstr,
            name, mask & IN_ISDIR ? "/" : " ");

    int spaces = (dirmap.max_dirpath_len - strlen(name)) + 3;

    for (int i = 0; i < spaces; i++)
        putchar(' ');

    fprintf(stdout, "%s%s\n", context_dir != NULL ? context_dir : "",
            context_dir != NULL ? "/" : "");

    dirwatch_free_event_info(&info);

    return true;
}

/* Handle a change event in the given directory. */
static void
dirwatch_on_event(struct inotify_event *event)
{
    if (event->len)
    {
        dirmap_entry_t *entry = dirmap_find_by_wd(&dirmap, event->wd);

        if (!dirwatch_log_event(event->mask, event->name,
                                entry == NULL ? "[Nothing]" : entry->dirpath))
            print_error(true, true, "unknown event in mask");
    }
}

/* Run an infinite loop to watch for the events in the given directory. */
static void
dirwatch_watch()
{
    while (true)
    {
        char buffer[EVENT_BUF_LEN];
        int length = read(config.fd, buffer, EVENT_BUF_LEN);

        if (length == -1)
            print_error(true, true,
                        "read from inotify file descriptor failed");

        for (int i = 0; i < length;)
        {
            struct inotify_event *event = (struct inotify_event *) &buffer[i];
            dirwatch_on_event(event);
            i += EVENT_SIZE + event->len;
        }
    }
}

/* Prints the usage of the program. If _exit is true, then it calls
   exit(EXIT_SUCCESS). */
static void
usage(bool _exit)
{
    fprintf(stdout, "Usage: %s [OPTIONS]... [DIRECTORY]\n\
Watches for changes in DIRECTORY. If no DIRECTORY is specified, it will watch \
the current directory.\n\
\n\
Options:\n\
  -e, --events=[EVENTS]...     Specify which events dirwatch should log.\n\
                                Valid events are (Event name - long specifier, short specifier):\n\n\
                                ALL EVENTS - all, 1\n\
                                CREATE     - create, c\n\
                                DELETE     - delete, d\n\
                                MODIFY     - modify, m\n\
                                READ       - read, r\n\
                                OPEN       - open, o\n\
                                CLOSE      - CLOSE, l\n\
                                ATTRCHANGE - attributes, a\n\
                                CWRITE     - cwrite, w\n\
                                NCRWRITE   - ncwrite, f\n\
                                MOVEDFROM  - mvfrom, v\n\
                                MOVEDTO    - mvto, t\n\
                                MOVE       - move, u\n\
                                DELSELF    - delself, s\n\
                                MVSELF     - mvself, e\n\n\
                                Multiple events can be seperated by commas (,).\n\
  -h, --help                   Show this help and exit.\n\
  -r, --recursive              Set watchers recursively to all directories and subdirectories under\n\
                                the given DIRECTORY.\n\
  -v, --version                Show the version of this program.\n\
  -V, --verbose=[LEVEL]        Enable verbose mode. LEVEL 1-3 are valid.\n\
                                If no LEVEL is specified, LEVEL 1 gets enabled.\n\
\n\
This program is a part of dirutils v%s.\n\
",
            PROGRAM_NAME, VERSION);

    if (_exit)
        exit(EXIT_SUCCESS);
}

/* Prints the version of the program. If _exit is true, then it calls
   exit(EXIT_SUCCESS). */
static void
version(bool _exit)
{
    fprintf(stdout, "%s version %s\n\
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

/* Parse the events specified by the user using the `-e` or `--events` option.
   Each event has it's own unique short and long identifier. Returns 0 if an
   unknown event identifier was found in the input. */
static uint32_t
dirwatch_parse_event_mask(char *input)
{
    char *delim = ",";
    char *token = strtok(input, delim);
    uint32_t mask = 0;

    while (token != NULL)
    {
        if (STREQ(token, "all") || STREQ(token, "1"))
            return IN_ALL_EVENTS;
        else if (STREQ(token, "create") || STREQ(token, "c"))
            mask |= IN_CREATE;
        else if (STREQ(token, "delete") || STREQ(token, "d"))
            mask |= IN_DELETE;
        else if (STREQ(token, "modify") || STREQ(token, "m"))
            mask |= IN_DELETE;
        else if (STREQ(token, "read") || STREQ(token, "r"))
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
        else if (STREQ(token, "mvfrom") || STREQ(token, "v"))
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

/* Start of the program and argument parsing. */
int
main(int argc, char **argv)
{
    set_program_name(argv[0]);

    config.mask = IN_DEFAULT; /* Default mask. */
    config.recursive = false;

    while (true)
    {
        int option_index;
        int c = getopt_long(argc, argv, "hve:Vr", long_options, &option_index);

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
                    print_error(false, true,
                                "invalid events specified.\nRun `%s --help' "
                                "for more detailed information.",
                                PROGRAM_NAME);

                config.mask = mask;
            }
            break;

            case 'V':
                config.verbosity
                    = (verbosity_t) (optarg == NULL ? 1 : atoi(optarg));

                if (config.verbosity < 0 || config.verbosity > 3)
                {
                    print_error(false, true,
                                "invalid verbosity level provided");
                }

                printf("WARNING: verbose mode was enabled (level %d)\n",
                       config.verbosity);
                break;

            case 'r':
                config.recursive = true;
                break;

            case '?':
                fprintf(stderr,
                        "Run `%s --help' for more detailed information.\n",
                        PROGRAM_NAME);
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

    return 0;
}
