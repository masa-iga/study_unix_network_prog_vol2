#include <cstdio>
#include <semaphore.h>
#include <pthread.h>

#define VPRINTF(...) \
    do { \
        printf("%s (L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

const int kBuff = 16;
const int kNitems = 1000;
const char *kPathSemMutex = "sem_mutex";
const char *kPathSemNempty = "sem_nempty";
const char *kPathSemNstored = "sem_nstored";

void *produce(void *arg);
void *consume(void *arg);

static int g_nitems;

static struct
{
    int     buff[kBuff];
    sem_t   *mutex, *nempty, *nstored;
} g_shared;


int main(int argc, char* argv[])
{
    g_nitems = kNitems;

    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    g_shared.mutex = sem_open(kPathSemMutex, O_CREAT | O_EXCL, fmode, 1);
    g_shared.nempty = sem_open(kPathSemNempty, O_CREAT | O_EXCL, fmode, kBuff);
    g_shared.nstored = sem_open(kPathSemNstored, O_CREAT | O_EXCL, fmode, 0);

    pthread_t tid_produce, tid_consume;
    pthread_setconcurrency(2);

    pthread_create(&tid_produce, NULL /* attr */, produce, NULL /* arg */);
    pthread_create(&tid_consume, NULL /* attr */, consume, NULL /* arg */);

    /* wait until the threads finishes */
    pthread_join(tid_produce, NULL /* retval */);
    pthread_join(tid_consume, NULL /* retval */);

    sem_unlink(kPathSemMutex);
    sem_unlink(kPathSemNempty);
    sem_unlink(kPathSemNstored);

    return 0;
}


void *produce(void *arg)
{
    VPRINTF("producer started\n");

    for (int i = 0; i < g_nitems; i++)
    {
        sem_wait(g_shared.nempty);
        sem_wait(g_shared.mutex);

        g_shared.buff[i % kBuff] = i;

        sem_post(g_shared.mutex);
        sem_post(g_shared.nstored);
    }

    return (NULL);
}


void *consume(void *arg)
{
    VPRINTF("consumer started\n");

    for (int i = 0; i < g_nitems; i++)
    {
        sem_wait(g_shared.nstored);
        sem_wait(g_shared.mutex);

        if (g_shared.buff[i % kBuff] != i)
        {
            printf("mismatch. g_shared.buff[%d] = %d\n", i, g_shared.buff[i]);
        }

        sem_post(g_shared.mutex);
        sem_post(g_shared.nempty);
    }

    return (NULL);
}
