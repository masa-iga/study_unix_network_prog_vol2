#include <sys/mman.h> // shm_open()
#include <sys/stat.h> // shm_open()
#include <fcntl.h> // shm_open()
#include <unistd.h> // ftruncate()
#include <sys/types.h> // ftruncate()
#include <semaphore.h> // sem_open()

struct shmstruct
{
    int count;
};

sem_t* mutex;

int main()
{
    const char* shmPath = "/tmp/shm_shared_counter";
    const char* mutexPath = "/tmp/mutex_shared_counter";

    shm_unlink(shmPath);

    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    int fd = shm_open(shmPath, O_RDWR | O_CREAT | O_EXCL, fmode);

    ftruncate(fd, sizeof(struct shmstruct));

    mmap(NULL, sizeof(struct shmstruct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    close(fd);


    const unsigned int initVal = 1;

    shm_unlink(mutexPath);
    mutex = sem_open(mutexPath, O_CREAT | O_EXCL, fmode, initVal);
    sem_close(mutex);

    return 0;
}
