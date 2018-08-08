#include <cstdio>
#include <semaphore.h>
#include <pthread.h>

#define VPRINTF(...) \
    do { \
        printf("%s (L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

void *produce(void *arg);
void *consume(void *arg);

const char *kPathSemMutex = "sem_mutex";
const char *kPathSemNempty = "sem_nempty";
const char *kPathSemNstored = "sem_nstored";

const int kMaxThreads = 100;
const int kNumProducers = 5;
const int kNumBuff = 10;
const int kNumItems = 1000;


struct
{
    int     buff[kNumBuff];
    int     nput;
    int     nputval;
    sem_t   *mutex, *nempty, *nstored;
} g_shared;


int main(int argc, char* argv[])
{
    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    g_shared.mutex = sem_open(kPathSemMutex, O_CREAT | O_EXCL, fmode, 1);
    g_shared.nempty = sem_open(kPathSemNempty, O_CREAT | O_EXCL, fmode, kNumBuff);
    g_shared.nstored = sem_open(kPathSemNstored, O_CREAT | O_EXCL, fmode, 0);


    /* All of producers and one consumer */
    pthread_setconcurrency(kNumProducers + 1);

    int count[kMaxThreads];
    pthread_t tid_produce[kMaxThreads];

    for (int i = 0; i < kNumProducers; i++)
    {
        count[i] = 0;
        pthread_create(&tid_produce[i], NULL /* attr */, produce, &count[i] /* arg */);
    }

    pthread_t tid_consume;

    pthread_create(&tid_consume, NULL, consume, NULL);

    for (int i = 0; i < kNumProducers; i++)
    {
        pthread_join(tid_produce[i], NULL /* retval */);
        VPRINTF("count[%d] = %d\n", i, count[i]);
    }

    pthread_join(tid_consume, NULL);

    sem_unlink(kPathSemMutex);
    sem_unlink(kPathSemNempty);
    sem_unlink(kPathSemNstored);

    return 0;
}


void *produce(void *arg)
{
    do
    {
        sem_wait(g_shared.nempty);
        sem_wait(g_shared.mutex);

        if (g_shared.nput >= kNumItems)
        {
            sem_post(g_shared.nempty);
            sem_post(g_shared.mutex);
            return (NULL);
        }

        g_shared.buff[g_shared.nput % kNumBuff] = g_shared.nputval;
        g_shared.nput++;
        g_shared.nputval++;

        sem_post(g_shared.mutex);
        sem_post(g_shared.nstored);

        *(static_cast<int*>(arg)) += 1;

    } while (true);

    return (NULL);
}


void *consume(void *arg)
{
    for (int i = 0; i < kNumItems; i++)
    {
        sem_wait(g_shared.nstored);
        sem_wait(g_shared.mutex);

        if (g_shared.buff[i % kNumBuff] != i)
        {
            VPRINTF("error: buff[%d] = %d\n", i, g_shared.buff[i % kNumBuff]);
        }

        sem_post(g_shared.mutex);
        sem_post(g_shared.nstored);
    }

    return (NULL);
}

// todo: read the book
