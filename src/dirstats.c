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
} dirstats_t;

typedef struct {
    bool recursive;
} dirstats_config_t;

static const struct option long_options[] = {
    { "recursive", no_argument, NULL, 'r' },
    { NULL,        0,           NULL,  0  } 
};

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

    int childcount = 0, filecount = 0, dircount = 0, linkcount = 0;
    struct dirent *dirent;

    while ((dirent = readdir(dir)) != NULL) 
    {
        if (STREQ(dirent->d_name, ".") || STREQ(dirent->d_name, "..")) 
            continue;

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

                if (!get_dirstats(dirent->d_name, &stats, config)) {
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

    return true;
}

static void print_dirstats(dirstats_t *stats)
{
    printf(
        COLOR("32", "%d") " files, " 
        COLOR("33", "%d") " directories, " 
        COLOR("34", "%d") " links and " 
        COLOR("1", "%d") " total.\n", stats->filecount, 
        stats->dircount, stats->linkcount, stats->childcount);
}

int main(int argc, char **argv)
{
    PROGRAM_NAME = argv[0];

    dirstats_config_t config = { 0 };

    while (true) 
    {
        int option_index;
        int c = getopt_long(argc, argv, "r", long_options, &option_index);

        if (c == -1)
            break;

        switch (c)
        {
            case 'r':
                config.recursive = true;
            break;

            case '?':
                exit(-1);

            default:
            break;
        }
    }

    dirstats_t stats;
    
    if (!get_dirstats(".", &stats, &config))
    {
        print_error(true, "cannot open directory");
    }

    print_dirstats(&stats);

    return 0;
}
