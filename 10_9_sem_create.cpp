#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h> // for O_* constants
#include <sys/stat.h> // for mode constants
#include <semaphore.h>

#include "../study_unix_network_prog_vol1/util.h"

/* 
   The variable 'optind' is the index of the next element to be processed in argv.
   The system initializes this value to 1.
   The caller can reset it to 1 to restart scanning of the same argv, or when scanning a new argument vector.
 */

int main(int argc, char *argv[])
{
    int c;
    const char *optstring = "ei:";
    int flags = O_RDWR | O_CREAT;
    unsigned int init_value = 0;

    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        switch (c)
        {
            case 'e':
                {
                    flags |= O_EXCL;
                    break;
                }
            case 'i':
                {
                    init_value = atoi(optarg);
                    break;
                }
            default:
                {
                    break;
                }
        }
    }

    if (optind != argc - 1)
    {
        err_quit("usage: semcreate [ -e ] [ -i initialvalue ] <name>");
    }

    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    sem_t *sem = sem_open(argv[optind], flags, fmode, init_value);

    sem_close(sem);

    return 0;
}
