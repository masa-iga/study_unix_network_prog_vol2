#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h> /* mode */
#include <fcntl.h> /* O_* constants */
#include <unistd.h>
#include <sys/types.h>
#include <cstdlib>

#include <sys/wait.h>
#include <sys/types.h>

#define VPRINTF(...) \
    do { \
        printf("L%d %s() ", __LINE__, __func__); \
        printf(__VA_ARGS__); \
    } while(false)

int main()
{

    const char* fname1 = "shmtest";
    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

    shm_unlink(fname1);
    int fd1 = shm_open(fname1, O_RDWR | O_CREAT | O_EXCL, fmode);

    if (fd1 == -1)
    {
        VPRINTF("error. shm_open() failed.");
        exit(1);
    }

    ftruncate(fd1, sizeof(int));

    const char* fname2 = "./_temp_12_10.txt";
    int fd2 = open(fname2, O_RDONLY);

    if (fd2 == -1)
    {
        VPRINTF("error. shm_open() failed.");
        exit(1);
    }

    struct stat stat;
    fstat(fd2, &stat);

    pid_t childpid;
    int *ptr1, *ptr2;

    if ((childpid = fork()) == 0) // child process
    {
        ptr2 = reinterpret_cast<int*>(mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd2, 0));
        ptr1 = reinterpret_cast<int*>(mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                    MAP_SHARED, fd1, 0));

        VPRINTF("[child] shm ptr = %p, motd ptr = %p\n", ptr1, ptr2);

        sleep(3);

        VPRINTF("shared memory integer = %d\n", *ptr1);

        exit(0);
    }

    /* parent process */
    ptr1 = reinterpret_cast<int*>(mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, fd1, 0));
    ptr2 = reinterpret_cast<int*>(mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd2, 0));

    VPRINTF("[parent] shm ptr = %p, motd ptr = %p\n", ptr1, ptr2);

    *ptr1 = 123;

    int* const status = NULL;
    const int options = 0;
    waitpid(childpid, status, options);

    return 0;
}
