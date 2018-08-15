#include <cstdio>
#include <sys/types.h> // for open()
#include <sys/stat.h> // for open()
#include <fcntl.h> // for open()
#include <unistd.h> // for read()
#include <semaphore.h>
#include <pthread.h>
#include <cstdint>
#include <stdarg.h>
#include <string>

#define VPRINTF(...) \
    do { \
        printf("%s(L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)


void *produce(void *arg);
void *consume(void *arg);

const char* kFileName = "10_33_input";
const char* kPathSemMutex = "sem_mutex";
const char* kPathSemNempty = "sem_nempty";
const char* kPathSemNstored = "sem_nstored";

const int kNumBuff = 2;
const int kSizeBuff = 8;

struct
{
    struct {
        char    data[kSizeBuff];
        ssize_t n;
    } buff[kNumBuff];

    sem_t   *mutex, *nempty, *nstored;
} g_shared;

int g_fd;


int main(int argc, char *argv[])
{
    g_fd = open(kFileName, O_RDONLY);

    if (g_fd == -1)
    {
        perror("open() failed: ");
        return -1;
    }

    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    g_shared.mutex = sem_open(kPathSemMutex, O_CREAT | O_EXCL, fmode, 1);
    g_shared.nempty = sem_open(kPathSemNempty, O_CREAT | O_EXCL, fmode, kNumBuff);
    g_shared.nstored = sem_open(kPathSemNstored, O_CREAT | O_EXCL, fmode, 0);

    pthread_setconcurrency(2);

    pthread_t tid_produce, tid_consume;

    pthread_create(&tid_produce, NULL /* attr */, produce, NULL /* arg */);
    pthread_create(&tid_consume, NULL /* attr */, consume, NULL /* arg */);

    pthread_join(tid_produce, NULL /* retval */);
    pthread_join(tid_consume, NULL /* retval */);

    sem_unlink(kPathSemMutex);
    sem_unlink(kPathSemNempty);
    sem_unlink(kPathSemNstored);

    return 0;
}


void *produce(void *arg)
{
    uint32_t idx = 0;

    while (true)
    {
        sem_wait(g_shared.nempty);
        sem_wait(g_shared.mutex);

        /* critical region */

        sem_post(g_shared.mutex);

        g_shared.buff[idx].n = read(g_fd, g_shared.buff[idx].data, kSizeBuff);

        if (g_shared.buff[idx].n == 0)
        {
            sem_post(g_shared.nstored);
            return (NULL);
        }

        idx = (idx + 1) % kNumBuff;

        sem_post(g_shared.nstored);
    }

    return (NULL);
}


void *consume(void *arg)
{
    uint32_t idx = 0;

    while (true)
    {
        sem_wait(g_shared.nstored);
        sem_wait(g_shared.mutex);

        /* critical region */

        sem_post(g_shared.mutex);

        if (g_shared.buff[idx].n == 0)
        {
            return (NULL);
        }

        write(STDOUT_FILENO, g_shared.buff[idx].data, g_shared.buff[idx].n);

        idx = (idx + 1) % kNumBuff;

        sem_post(g_shared.nempty);
    }

    return (NULL);
}
