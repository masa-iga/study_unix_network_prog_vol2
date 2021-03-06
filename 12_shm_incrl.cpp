#include <cstdio>
#include <cstdlib>
#include <semaphore.h>
#include <unistd.h> // for fork()
#include <sys/mman.h> // for mmap()
#include <algorithm>

using namespace std;

const char* kSemName = "/tmp/mysem";
static int g_count = 0;
const int g_nloop = 100;

#define TEST_BASED_SEMAPHORE (0)

#define VPRINTF(...) \
    do {\
        printf("L%d %s(): ", __LINE__, __func__);\
        printf(__VA_ARGS__);\
    } while(false); \


/*
   create semaphore in order to avoid racing condition when incrementing g_count, defined a global variable,
   from both parent and child processes.
   This is wrong; parent and child process will have each own g_count variable.
*/
void fork_12_03()
{
    VPRINTF("start\n");

    /* init */
    g_count = 0;

    /* create semaphore, init and unlink */
    sem_t *mutex;
    mutex = sem_open(kSemName, O_CREAT | O_EXCL, 0644, 1);

    /* remove the named semaphore; even if the program stops, it can keep clean */
    if (sem_unlink(kSemName) == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    /* set no buffering mode to stdout */
    setbuf(stdout, NULL);

    /* child process */
    if (fork() == 0)
    {
        for (int i = 0; i < g_nloop; i++)
        {
            sem_wait(mutex);
            VPRINTF("child: %d\n", g_count++);
            sem_post(mutex);
        }

        exit(0);
    }

    /* parent process */
    for (int i = 0; i < g_nloop; i++)
    {
        sem_wait(mutex);
        VPRINTF("parent: %d\n", g_count++);
        sem_post(mutex);
    }

    sleep(1);
}

/* 
   this is a program how to share data between parent and child processes with mmap()
*/

void mmap_12_10()
{
    VPRINTF("start\n");

    /* init */
    g_count = 0;

    /* file open, initialized, memory map */
    int *ptr;

    {
        const char* inpf = "./_temp_12_10.txt";
        const int zero = 0;

        int fd = open(inpf, O_RDWR | O_CREAT, 0644);
        write(fd, &zero, sizeof(int));

        const off_t offset = 0;

        /* MAP_SHARED enables to share this mapping between parent and child processes */
        ptr = reinterpret_cast<int*>(mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset));
        close(fd);
    }

    /* create semaphore, init and unlink */
    sem_t *mutex;
    mutex = sem_open(kSemName, O_CREAT | O_EXCL, 0644, 1);

    /* remove the named semaphore; even if the program stops, it can keep clean */
    if (sem_unlink(kSemName) == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    /* set no buffering mode to stdout */
    setbuf(stdout, NULL);

    /* child process */
    /* fork() copies memory mapping to child process parent has */
    if (fork() == 0)
    {
        for (int i = 0; i < g_nloop; i++)
        {
            sem_wait(mutex);
            VPRINTF("child: %d\n", (*ptr)++);
            sem_post(mutex);
        }

        exit(0);
    }

    /* parent process */
    for (int i = 0; i < g_nloop; i++)
    {
        sem_wait(mutex);
        VPRINTF("parent: %d\n", (*ptr)++);
        sem_post(mutex);
    }
}


#if TEST_BASED_SEMAPHORE
void memory_base_semaphore_12_12()
{
    VPRINTF("start\n");

    struct Shared {
        sem_t   mutex;
        int     count;
    } shared;

    /* open file, initialize to 0, map into memory */
    Shared* p_shared;

    {
        const char* const inpf = "./_temp_12_10.txt";

        int fd = open(inpf, O_RDWR | O_CREAT, 0644);
        write(fd, &shared, sizeof(struct Shared));

        void* const p_addr = NULL;
        const off_t offset = 0;
        p_shared = reinterpret_cast<Shared*>(mmap(p_addr, sizeof(struct Shared),
                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset));
        close(fd);
    }

    /* initialize semaphore that is shared between processes */
    {
        const int pshared = 1; // shared between processes
        const unsigned int value = 1; // initialized value

        /* Mac OS doesn't support sem_init() already... */
        sem_init(&p_shared->mutex, pshared, value);
    }

    char* const buf = NULL; // the size of buffer; here, no buffer is good
    setbuf(stdout, buf);

    const int nloop = 200;

    /* child */
    if (fork() == 0)
    {
        for (int i = 0; i < nloop; i++)
        {
            sem_wait(&p_shared->mutex);
            VPRINTF("child: %d\n", ++p_shared->count);
            sem_post(&p_shared->mutex);
        }

        exit(0);
    }

    /* parent*/
    for (int i = 0; i < nloop; i++)
    {
        sem_wait(&p_shared->mutex);
        VPRINTF("parent: %d\n", ++p_shared->count);
        sem_post(&p_shared->mutex);
    }

    exit(0);
}
#endif // TEST_BASED_SEMAPHORE


/*
   we can skip creating a file step if your purpose is to share data between parent and child process.
   Let's use MAP_ANONYMOUS option in mmap().
 */
void anonymous_12_14()
{

    int* ptr;

    {
        void* const p_addr = NULL;
        const int fd = -1; // must be -1 if use MAP_ANONYMOUS
        const off_t offset = 0;

        ptr = reinterpret_cast<int*>(mmap(p_addr, sizeof(int), PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, fd, offset));
    }

    sem_t *mutex;
    mutex = sem_open(kSemName, O_CREAT | O_EXCL, 0644, 1);

    if (sem_unlink(kSemName) == -1)
    {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    setbuf(stdout, NULL);

    /* child process */
    /* fork() copies memory mapping to child process parent has */
    if (fork() == 0)
    {
        for (int i = 0; i < g_nloop; i++)
        {
            sem_wait(mutex);
            VPRINTF("child: %d\n", (*ptr)++);
            sem_post(mutex);
        }

        exit(0);
    }

    /* parent process */
    for (int i = 0; i < g_nloop; i++)
    {
        sem_wait(mutex);
        VPRINTF("parent: %d\n", (*ptr)++);
        sem_post(mutex);
    }

}


void mmap_size_test_12_15(int filesize, int mmapsize)
{
    VPRINTF("file size %d (0x%x)  mmap size %d (0x%x)\n", filesize, filesize, mmapsize, mmapsize);

    /* file open: create a new file or cut down an existing file, and set file size */
    uint8_t* ptr;

    {
        const char* const inpf = "./_temp_12_10.txt";
        const size_t count = 1;

        int fd = open(inpf, O_RDWR | O_CREAT | O_TRUNC, 0644);
        lseek(fd, filesize - 1, SEEK_SET);
        write(fd, "0", count);

        const off_t offset = 0;
        ptr = reinterpret_cast<uint8_t*>(mmap(NULL, mmapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset));
        close(fd);
    }

    const int pagesize = sysconf(_SC_PAGESIZE);
    VPRINTF("page size %d\n", pagesize);

    int idx;
    for (idx = 0; idx < max(filesize, mmapsize); idx += pagesize)
    {
        VPRINTF("ptr[%d] = %d\n", idx, ptr[idx]);
        ptr[idx] = 1;

        VPRINTF("ptr[%d] = %d\n", idx + pagesize - 1, ptr[idx + pagesize - 1]);
        ptr[idx + pagesize - 1] = 1;
    }

    VPRINTF("ptr[%d] = %d\n", idx, ptr[idx]);

}


/*
   This explains how to handle mapping a file which of size has been creasing.
   If you don't care the size, you will face an error of Segmantation fault or Bus error,
   tested in mmap_size_test_12_15().
*/
void mmap_incr_size_12_19()
{
    int fd;

    {
        const char* const inpf = "./_temp_12_10.txt";
        const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // permission 0644

        fd = open(inpf, O_RDWR | O_CREAT | O_TRUNC, fmode); // O_TRUNC trancates the size to 0
    }

    char* ptr;
    const size_t mapsize_in_bytes = 32768;

    {
        const off_t offset = 0;
        ptr = reinterpret_cast<char*>(mmap(NULL, mapsize_in_bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset));
    }

    for (int i = 0; i <= mapsize_in_bytes; i+= 4096)
    {
        VPRINTF("setting file size to %d\n", i);

        const off_t len = i;
        ftruncate(fd, len); // helps to handle a memmory mapping and its access

        VPRINTF("ptr[%d] = %d\n", i-1, ptr[i-1]);
    }

}


int main(int argc, char **argv)
{
    const bool run_fork_12_03 = false;
    const bool run_mmap_12_10 = false;
    const bool run_memory_base_semaphore_12_12 = false;
    const bool run_anonymous_12_14 = false;
    const bool run_mmap_size_test_12_15 = false;
    const bool run_mmap_incr_size_12_19 = true;

    if (run_fork_12_03)
        fork_12_03();

    if (run_mmap_12_10)
        mmap_12_10();

    /* it looks memory based semaphore doesn't work well in MacOS */
    if (run_memory_base_semaphore_12_12)
#if TEST_BASED_SEMAPHORE
        memory_base_semaphore_12_12();
#endif // TEST_BASED_SEMAPHORE

    if (run_anonymous_12_14)
        anonymous_12_14();

    if (run_mmap_size_test_12_15)
    {
        //mmap_size_test_12_15(5000, 5000);
        mmap_size_test_12_15(5000, 15000);
    }

    if (run_mmap_incr_size_12_19)
        mmap_incr_size_12_19();

    return 0; 
}

