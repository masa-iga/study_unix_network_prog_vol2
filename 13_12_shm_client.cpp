#include <sys/mman.h> // shm_open()
#include <sys/stat.h> // shm_open()
#include <fcntl.h> // shm_open()
#include <sys/types.h> // mode_t
#include <cstdlib> // NULL
#include <cstdio> // snprintf()
#include <unistd.h> // close()
#include <semaphore.h>
#include <errno.h> // int errno
#include <string.h> // strncpy()

#include "13_10_shmstruct.h"

#define VPRINTF(...) \
    do { \
        printf("%s() L%d: ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

int main()
{
    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    int fd = shm_open(kPathShm, O_RDWR, fmode);

    struct shmStruct* pShm = reinterpret_cast<struct shmStruct*>(
            mmap(NULL, sizeof(struct shmStruct), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0));

    if (pShm == (void*)-1)
    {
        perror("mmap() failed.");
    }

    close(fd);


    semStruct sems;

    sems.mutex = sem_open(kPathSemMutex, 0);

    if (sems.mutex == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sems.nempty= sem_open(kPathSemEmpty, 0);

    if (sems.nempty == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sems.nstored = sem_open(kPathSemStore, 0);

    if (sems.nstored == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sems.noverflowmutex = sem_open(kPathSemOverflow, 0);

    if (sems.noverflowmutex == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }


    const int nloop = 100;
    const int nusec = 1000;
    const pid_t pid = getpid();

    for (int i = 0; i < nloop; i++)
    {
        char mesg[MESGSIZE];

        usleep(nusec);
        snprintf(mesg, MESGSIZE, "pid %ld: message %d", (long)pid, i);

        if (sem_trywait(sems.nempty) == -1)
        {
            if (errno == EAGAIN) // timeout case
            {
                sem_wait(sems.noverflowmutex);
                pShm->noverflow++;
                sem_post(sems.noverflowmutex);

                continue;
            }
            else
            {
                perror("sem_trywait error");
            }
        }

        sem_wait(sems.mutex);
        const long offset = pShm->msgoff[pShm->nput];

        if (++(pShm->nput) >= NMESG) // ring buffer
            pShm->nput = 0;

        sem_post(sems.mutex);

        strncpy(&pShm->msgdata[offset], mesg, MESGSIZE);
        sem_post(sems.nstored);
    }

    return 0;
}
