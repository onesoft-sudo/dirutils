/* 
TODO: Show file sizes
*/

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
    verbosity_t verbosity;
} dirstats_config_t;

static const struct option long_options[] = {
    { "recursive", no_argument,       NULL, 'r' },
    { "all",       no_argument,       NULL, 'a' },
    { "verbose",   optional_argument, NULL, 'V' },
    { "help",      no_argument,       NULL, 'h' },
    { NULL,        0,                 NULL,  0  } 
};

static dirstats_config_t config;

void usage(int status) 
{
    fprintf(stdout, "Usage: %s [OPTION]... [DIRECTORY]\n", PROGRAM_NAME);

    if (status != 0)
        exit(status);
}

static bool get_dirstats(char *dirpath, dirstats_t *destptr, dirstats_config_t *config, char **error_path) 
{
    *error_path = NULL;

    DIR *dir = opendir(dirpath);

    if (dir == NULL) 
    {
        *error_path = strdup(dirpath);
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

                char *newpath = malloc(strlen(dirpath) + strlen(dirent->d_name) + 2);
                
                strcpy(newpath, dirpath);
                strcat(newpath, "/");
                strcat(newpath, dirent->d_name);

                newpath[strlen(dirpath) + strlen(dirent->d_name) + 1] = '\0';

                LOG_DEBUG_1(config->verbosity, "reading directory: %s\n", newpath);           

                if (newpath == NULL) {                    
                    return false;
                }

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

    config.verbosity = 0;
    
    while (true) 
    {
        int option_index;
        int c = getopt_long(argc, argv, "hraV", long_options, &option_index);

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

            case 'V':
                config.verbosity = (verbosity_t) (optarg == NULL ? 1 : atoi(optarg));

                if (config.verbosity < 0 || config.verbosity > 3) {
                    print_error(false, true, "invalid verbosity level provided");
                }

                printf("WARNING: Verbose mode was enabled (Level %d)\n", config.verbosity);
            break;

            case '?':
                exit(-1);

            default:
            break;
        }
    }

    dirstats_t stats = { 0, 0, 0, 0, 0 };

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
