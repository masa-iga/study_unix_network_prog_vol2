#include <sys/mman.h> // shm_unlink()
#include <sys/stat.h> // shm_unlink()
#include <fcntl.h> // shm_unlink()
#include <sys/types.h> // mode_t
#include "13_10_shmstruct.h"
#include <unistd.h> // fruncate()
#include <semaphore.h>
#include <cstdio>
#include <cstdlib>

#define VPRINTF(...) \
    do { \
        printf("%s() L%d: ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)


int main()
{
    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

    shm_unlink(kPathShm);
    int fd = shm_open(kPathShm, O_RDWR | O_CREAT | O_EXCL, fmode);

    if (fd == -1)
    {
        perror("shm_open() failed.");
    }

    ftruncate(fd, sizeof(struct shmStruct));

    struct shmStruct* pShm = reinterpret_cast<struct shmStruct*>(
            mmap(NULL, sizeof(struct shmStruct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    if (pShm == (void*)-1)
    {
        perror("mmap() failed.");
    }

    close(fd);

    VPRINTF("pShm %p\n", pShm);

    /* initialize for offsets */
    VPRINTF("initialize message offset\n");

    for (int i = 0; i < NMESG; i++)
    {
        pShm->msgoff[i] = i * MESGSIZE;
    }



    semStruct sems;


    /* initialize for semaphore */

    unsigned int val = 1;

    VPRINTF("set up semaphore\n");

    sem_unlink(kPathSemMutex);
    sems.mutex = sem_open(kPathSemMutex, O_CREAT | O_EXCL, fmode, val);

    if (sems.mutex == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    val = NMESG;

    sem_unlink(kPathSemEmpty);
    sems.nempty= sem_open(kPathSemEmpty, O_CREAT | O_EXCL, fmode, val);

    if (sems.nempty == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    VPRINTF("nempty 0x%p\n", sems.nempty);

    val = 0;

    sem_unlink(kPathSemStore);
    sems.nstored = sem_open(kPathSemStore, O_CREAT | O_EXCL, fmode, val);

    if (sems.nstored == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    val = 1;

    sem_unlink(kPathSemOverflow);
    sems.noverflowmutex = sem_open(kPathSemOverflow, O_CREAT | O_EXCL, fmode, val);

    if (sems.noverflowmutex == SEM_FAILED)
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    int index = 0;
    int lastnoverflow = 0;

    while (true)
    {
        sem_wait(sems.nstored);
        sem_wait(sems.mutex);

        const long offset = pShm->msgoff[index];
        VPRINTF("index = %2d: %s\n", index, &pShm->msgdata[offset]);

        if (++index >= NMESG) // ring buffer
            index = 0;

        sem_post(sems.mutex);
        sem_post(sems.nempty);


        sem_wait(sems.noverflowmutex);
        const int temp = pShm->noverflow;
        sem_post(sems.noverflowmutex); // do not call printf() in locking

        if (temp != lastnoverflow)
        {
            VPRINTF("noverflow = %d\n", temp);
            lastnoverflow = temp;
        }
    }

    return 0;
}
