#include <sys/mman.h> // shm_open()
#include <sys/stat.h> // shm_open()
#include <fcntl.h> // shm_open()
#include <unistd.h> // ftruncate()
#include <sys/types.h> // ftruncate()
#include <semaphore.h> // sem_open()
#include <cstdio>

#define VPRINTF(...) \
    do { \
        printf("%s() L%d: ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)


struct shmstruct
{
    int count;
};

sem_t* mutex;

int main()
{
    const char* shmPath = "/tmp/shm_shared_counter";
    const char* mutexPath = "/tmp/mutex_shared_counter";

    const int nloop = 100;

    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    int fd = shm_open(shmPath, O_RDWR, fmode);

    struct shmstruct* ptr = reinterpret_cast<struct shmstruct*>(
            mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    close(fd);


    const int oflag = 0;
    mutex = sem_open(mutexPath, oflag);

    pid_t pid = getpid();

    for (int i = 0; i < nloop; i++)
    {
        sem_wait(mutex);
        VPRINTF("[pid %ld] %d\n", (long)pid, ptr->count++);
        sem_post(mutex);
    }

    return 0;
}

