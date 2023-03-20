 /*
    dirstats.c -- show statistical information about directories.
    
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
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <getopt.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "utils.h" 

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE
#endif 

typedef struct {
    size_t filecount;
    size_t dircount;
    size_t linkcount;
    size_t childcount;
    size_t hiddencount;
    size_t dirsize;
} dirstats_t;

typedef struct {
    bool recursive;
    bool count_hidden_files;
    bool filesize;
    verbosity_t verbosity;
} dirstats_config_t;

static const struct option long_options[] = {
    { "recursive", no_argument,       NULL, 'r' },
    { "all",       no_argument,       NULL, 'a' },
    { "verbose",   optional_argument, NULL, 'V' },
    { "version",   no_argument,       NULL, 'v' },
    { "help",      no_argument,       NULL, 'h' },
    { "size",      no_argument,       NULL, 's' },
    { NULL,        0,                 NULL,  0  } 
};

static dirstats_config_t config;

static ssize_t get_file_size(char *filename, bool seek_back)
{
    size_t size;
    
#ifdef HAVE_SYS_STAT_H
    struct stat statresult;

    if (stat(filename, &statresult) != 0)
        return -1;

    size = statresult.st_size;
#else
    FILE *fp = fopen(filename, "r");

    if (fp == NULL)
        return -1;

    if (fseek(fp, 0, SEEK_END) != 0) 
        return -1;
     
    size = ftell(fp);

    if (seek_back)
        fseek(fp, 0, SEEK_SET);

    fclose(fp);
#endif

    return size;
}

static void usage(int status) 
{
    fprintf(stdout, "Usage: %s [OPTION]... [DIRECTORY]\n\
Show statistical information about the DIRECTORY. \
The current directory is the default.\n\n\
Options:\n\
  -a, --all                  Do not ignore hidden files/directories\n\
                              (files/directories starting with `.').\n\
  -h, --help                 Show this help and exit.\n\
  -r, --recursive            Recursively count files/directories and\n\
                              their sizes under DIRECTORY.\n\
  -s, --size                 Show size of DIRECTORY.\n\
  -V, --verbose=[LEVEL]      Enable verbose mode. LEVEL 1-3 are valid.\n\
                              If no LEVEL is specified, LEVEL 1 gets enabled.\n\
  -v, --version              Show the program version information.\n", PROGRAM_NAME);

    if (status != 0)
        exit(status);
}

static void show_version()
{
    fprintf(stdout, "%s version %s\n\
Copyright (C) 2023 OSN Inc.\n\
This program is licensed under GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
\n\
Written by Ar Rakin <rakinar2@onesoftnet.eu.org>.\n", PROGRAM_NAME, VERSION);
}

static bool get_dirstats(char *dirpath, dirstats_t *destptr, 
                            dirstats_config_t *config, char **error_path) 
{
    *error_path = NULL;

    DIR *dir = opendir(dirpath);

    if (dir == NULL) 
    {
        *error_path = strdup(dirpath);
        return false;
    }

    size_t childcount = 0, 
        filecount = 0, 
        dircount = 0, 
        linkcount = 0, 
        hiddencount = 0;

    size_t dirsize = 0;
    
    struct dirent *dirent;

    while ((dirent = readdir(dir)) != NULL) 
    {
        if (STREQ(dirent->d_name, ".") || STREQ(dirent->d_name, "..")) 
            continue;

        if (dirent->d_type == DT_REG && config->filesize) {
            if (dirent->d_name[0] != '.' || (dirent->d_name[0] == '.' && 
                config != NULL && config->count_hidden_files)) 
            {
                char *newpath = malloc(strlen(dirpath) + strlen(dirent->d_name) + 2);
                    
                if (newpath == NULL) 
                    return false;
                
                strcpy(newpath, dirpath);
                strcat(newpath, "/");
                strcat(newpath, dirent->d_name);

                size_t size = get_file_size(newpath, false);

                if (size == -1)
                {
                    LOG_DEBUG_1(config->verbosity, "ERROR calculating size of `%s'\n", newpath);
                    print_error(true, false, "cannot calculate size of `%s'", newpath);
                    free(newpath);
                    exit(EXIT_FAILURE);
                }

                if (config->filesize)
                {
                    LOG_DEBUG_2(config->verbosity, "Size: %zu bytes: %s\n", size, newpath);
                }
                
                free(newpath);
                dirsize += size;
            }
        }

        if (dirent->d_name[0] == '.')
        {
            hiddencount++;

            if (config == NULL || !config->count_hidden_files)
                continue;
        }

        if (dirent->d_type == DT_REG)
            filecount++;
        else if (dirent->d_type == DT_DIR) 
        {
            dircount++;

            if (config != NULL && config->recursive)
            {
                dirstats_t stats;

                char *newpath = malloc(strlen(dirpath) + strlen(dirent->d_name) + 2);

                if (newpath == NULL) 
                    return false;
                
                strcpy(newpath, dirpath);
                strcat(newpath, "/");
                strcat(newpath, dirent->d_name);

                newpath[strlen(dirpath) + strlen(dirent->d_name) + 1] = '\0';

                LOG_DEBUG_1(config->verbosity, "reading directory: %s\n", newpath);           

                if (!get_dirstats(newpath, &stats, config, error_path)) 
                {
                    LOG_DEBUG_3(config->verbosity, "ERROR reading directory: %s\n", newpath);
                    free(newpath);
                    return false;
                }

                LOG_DEBUG_2(config->verbosity, "successfully read directory: %s\n", newpath);           

                free(newpath);

                filecount += stats.filecount;
                childcount += stats.childcount;
                dircount += stats.dircount;
                linkcount += stats.linkcount;
                hiddencount += dirent->d_name[0] == '.' ? stats.childcount : stats.hiddencount;
                dirsize += stats.dirsize;
            }
        }
        else if (dirent->d_type == DT_LNK)
            linkcount++;

        childcount++;
    }

    closedir(dir);

    destptr->childcount = childcount;
    destptr->filecount = filecount;
    destptr->dircount = dircount;
    destptr->linkcount = linkcount;
    destptr->hiddencount = hiddencount;
    destptr->dirsize = dirsize;

    return true;
}

typedef struct {
    double value;
    char unit;
} format_size_t;

static format_size_t format_size(size_t size)
{
    format_size_t format;
    double tmpsize = 0.0 + size;

    if (tmpsize < 1024) 
    {
        format.unit = 'B';
    }
    else 
    {
        tmpsize /= 1024;

        if (tmpsize < 1024) 
        {
            format.unit = 'K';
        }
        else 
        {
            tmpsize /= 1024;
            
            if (tmpsize < 1024) 
            {
                format.unit = 'M';
            }
            else 
            {
                tmpsize /= 1024;
            
                if (tmpsize < 1024) 
                {
                    format.unit = 'M';
                }
                else 
                {
                    tmpsize /= 1024;
                
                    if (tmpsize < 1024) 
                    {
                        format.unit = 'G';
                    }
                    else 
                    {
                        tmpsize /= 1024;
                        format.unit = 'T';
                    }
                }            
            }
        }
    }

    format.value = tmpsize;

    return format;
}

static void print_dirstats(dirstats_t *stats)
{
    format_size_t format = format_size(stats->dirsize);

    printf(
        COLOR("32", "%zu") " file%s, " 
        COLOR("33", "%zu") " director%s, " 
        COLOR("34", "%zu") " link%s and " 
        COLOR("36", "%zu") " total.", stats->filecount, stats->filecount != 1 ? "s" : "",  
        stats->dircount, stats->dircount != 1 ? "ies" : "y", stats->linkcount,
        stats->linkcount != 1 ? "s" : "", stats->childcount);

    if (config.filesize)
        printf(" Calculated size is %.1lf%c.", format.value, format.unit);

    if (config.count_hidden_files) 
        printf(" Counting " COLOR("1", "%d") " hidden files.", stats->hiddencount);
    
    printf("\n");
}

int main(int argc, char **argv)
{
    set_program_name(argv[0]);

    config.verbosity = 0;
    
    while (true) 
    {
        int option_index;
        int c = getopt_long(argc, argv, "hraVs", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                usage(0);
                exit(0);

            case 'v':
                show_version();
                exit(0);

            case 'r':
                config.recursive = true;
            break;

            case 'a':
                config.count_hidden_files = true;
            break;

            case 's':
                config.filesize = true;
            break;

            case 'V':
                config.verbosity = (verbosity_t) (optarg == NULL ? 1 : atoi(optarg));

                if (config.verbosity < 0 || config.verbosity > 3) {
                    print_error(false, true, "invalid verbosity level provided");
                }

                printf("WARNING: verbose mode was enabled (level %d)\n", config.verbosity);
            break;

            case '?':
                exit(-1);

            default:
            break;
        }
    }

    dirstats_t stats = { 0, 0, 0, 0, 0, 0 };

    char *dirpath = ".";
    bool allocated = false;
    char *error_path = NULL;

    for (int i = optind; i < argc; i++)
    {
        dirpath = strdup(argv[i]);
        allocated = true;
        break;
    }

    LOG_DEBUG_1(config.verbosity, "reading directory: %s\n", dirpath);           

    if (!get_dirstats(dirpath, &stats, &config, &error_path))
    {
        LOG_DEBUG_3(config.verbosity, "ERROR reading directory: %s\n", dirpath); 
        print_error(true, false, "cannot open `%s'", error_path);

        if (allocated)
            free(dirpath);

        if (error_path != NULL)
            free(error_path);
        
        exit(EXIT_FAILURE);
    }

    LOG_DEBUG_2(config.verbosity, "successfully read directory: %s\n", dirpath); 

    if (allocated)
        free(dirpath);

    print_dirstats(&stats);

    return 0;
}
