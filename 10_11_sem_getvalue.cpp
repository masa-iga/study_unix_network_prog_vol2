#include <cstdio>
#include <semaphore.h>

#include "../study_unix_network_prog_vol1/util.h"

/* Note Mac OS x doesn't support sem_getvalue() anymore */

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        err_quit("usage: semgetvalue <name>");
    }

    sem_t* sem = sem_open(argv[1], 0);

    int val;
    sem_getvalue(sem, &val);

    printf("value = %d\n", val);

    return 0;
}
