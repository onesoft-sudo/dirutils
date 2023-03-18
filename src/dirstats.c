#include <stdio.h>
#include <stdlib.h> 
#include <stdbool.h> 
#include <string.h> 
#include <dirent.h>  
#include <getopt.h>  
#include "utils.h" 

typedef struct {
    int filecount;
    int dircount;
    int linkcount;
    int childcount;
    int hiddencount;
} dirstats_t;

typedef struct {
    bool recursive;
    bool count_hidden_files;
} dirstats_config_t;

static const struct option long_options[] = {
    { "recursive", no_argument, NULL, 'r' },
    { "all",       no_argument, NULL, 'a' },
    { "help",      no_argument, NULL, 'h' },
    { NULL,        0,           NULL,  0  } 
};

static dirstats_config_t config;

void usage(int status) 
{
    fprintf(stdout, "Usage: %s [OPTION]... [DIRECTORY]\n", PROGRAM_NAME);

    if (status != 0)
        exit(status);
}

static bool get_dirstats(char *dirpath, dirstats_t *destptr, dirstats_config_t *config) 
{
    DIR *dir = opendir(dirpath);

    if (dir == NULL) 
    {
        return false;
    }

    int childcount = 0, 
        filecount = 0, 
        dircount = 0, 
        linkcount = 0, 
        hiddencount = 0;
    
    struct dirent *dirent;

    while ((dirent = readdir(dir)) != NULL) 
    {
        if (STREQ(dirent->d_name, ".") || STREQ(dirent->d_name, "..")) 
            continue;

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
                char *newpath = strcat_malloc(dirpath, "/", dirent->d_name, NULL);

                if (newpath == NULL)
                    return false;

                if (!get_dirstats(dirent->d_name, &stats, config)) 
                {
                    free(newpath);
                    return false;
                }

                free(newpath);

                filecount += stats.filecount;
                childcount += stats.childcount;
                dircount += stats.dircount;
                linkcount += stats.linkcount;
            }
        }
        else if (dirent->d_type == DT_LNK)
            linkcount++;

        childcount++;
    }

    destptr->childcount = childcount;
    destptr->filecount = filecount;
    destptr->dircount = dircount;
    destptr->linkcount = linkcount;
    destptr->hiddencount = hiddencount;

    return true;
}

static void print_dirstats(dirstats_t *stats)
{
    printf(
        COLOR("32", "%d") " file%s, " 
        COLOR("33", "%d") " director%s, " 
        COLOR("34", "%d") " link%s and " 
        COLOR("36", "%d") " total.", stats->filecount, stats->filecount != 1 ? "s" : "",  
        stats->dircount, stats->dircount != 1 ? "ies" : "y", stats->linkcount,
        stats->linkcount != 1 ? "s" : "", stats->childcount);

    if (config.count_hidden_files) 
    {
        printf(" (Counting " COLOR("1", "%d") " hidden files)", stats->hiddencount);
    }

    printf("\n");
}

int main(int argc, char **argv)
{
    PROGRAM_NAME = argv[0];

    while (true) 
    {
        int option_index;
        int c = getopt_long(argc, argv, "hra", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'h':
                usage(0);
                exit(0);

            case 'r':
                config.recursive = true;
            break;

            case 'a':
                config.count_hidden_files = true;
            break;

            case '?':
                exit(-1);

            default:
            break;
        }
    }

    dirstats_t stats;

    char *dirpath = ".";

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
        {
            dirpath = argv[i];
            break;
        }
    }
    
    if (!get_dirstats(dirpath, &stats, &config))
    {
        print_error(true, "cannot open `%s'", dirpath);
    }

    print_dirstats(&stats);

    return 0;
}
