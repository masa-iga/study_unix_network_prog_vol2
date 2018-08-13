#include <cstdio>
#include <semaphore.h>
#include <pthread.h>

#define VPRINTF(...) \
    do { \
        printf("%s(L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

const char *kPathSemMutex = "sem_mutex";
const char *kPathSemNempty = "sem_nempty";
const char *kPathSemNstored= "sem_nstored";

const int kNumBuff = 10;
const int kNumProducers = 4;
const int kNumConsumers = 6;
const int kMaxThreads = 8;
const int kNumItems = 1000;

struct
{
    int     buff[kNumBuff];
    int     nput;
    int     nputval;
    int     nget;
    int     ngetval;
    sem_t   *mutex, *nempty, *nstored;
} g_shared;

void *produce(void *arg);
void *consume(void *arg);


int main()
{
    /* Initialize global variable;
       Propably compiler guaranteed that global varialbes will be initialized zero */
    {
        g_shared.nput = 0;
        g_shared.nputval = 0;
        g_shared.nget = 0;
        g_shared.ngetval = 0;
    }

    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    sem_unlink(kPathSemMutex);
    sem_unlink(kPathSemNempty);
    sem_unlink(kPathSemNstored);

    g_shared.mutex = sem_open(kPathSemMutex, O_CREAT | O_EXCL, fmode, 1);
    g_shared.nempty = sem_open(kPathSemNempty, O_CREAT | O_EXCL, fmode, kNumBuff);
    g_shared.nstored = sem_open(kPathSemNstored, O_CREAT | O_EXCL, fmode, 0);

    static_assert(kNumProducers <= kMaxThreads, "Number of producer is over the max");
    static_assert(kNumConsumers <= kMaxThreads, "Number of consumer is over the max");

    pthread_t tid_produce[kNumProducers], tid_consume[kNumConsumers];
    int prodcount[kNumProducers], conscount[kNumConsumers];

    pthread_setconcurrency(kNumProducers + kNumConsumers);

    for (int i = 0; i < kNumProducers; i++)
    {
        prodcount[i] = 0;
        pthread_create(&tid_produce[i], NULL /* attr */, produce, &prodcount[i] /* arg */);
    }

    for (int i = 0; i < kNumConsumers; i++)
    {
        conscount[i] = 0;
        pthread_create(&tid_consume[i], NULL /* attr */, consume, &conscount[i] /* arg */);
    }

    /* Wait until all producers terminate */
    for (int i = 0; i < kNumProducers; i++)
    {
        pthread_join(tid_produce[i], NULL /* retval */);
        VPRINTF("prodcount[%d] = %d\n", i, prodcount[i]);
    }

    /* Wait until all consumers terminate */
    for (int i = 0; i < kNumConsumers; i++)
    {
        pthread_join(tid_consume[i], NULL /* retval */);
        VPRINTF("condcount[%d] = %d\n", i, conscount[i]);
    }

    sem_unlink(kPathSemMutex);
    sem_unlink(kPathSemNempty);
    sem_unlink(kPathSemNstored);

    return 0;
}


void *produce(void *arg)
{
    do {

        sem_wait(g_shared.nempty);
        sem_wait(g_shared.mutex);

        if (g_shared.nput >= kNumItems)
        {
            sem_post(g_shared.nstored); /* let consumers terminate */
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
    do {

        sem_wait(g_shared.nstored);
        sem_wait(g_shared.mutex);

        if (g_shared.nget >= kNumItems)
        {
            sem_post(g_shared.nstored);
            sem_post(g_shared.mutex);
            return (NULL);
        }

        const int idx = g_shared.nget % kNumBuff;

        if (g_shared.buff[idx] != g_shared.ngetval)
        {
            VPRINTF("error: buff[%d] = %d\n", idx, g_shared.buff[idx]);
            return (NULL);
        }

        g_shared.nget++;
        g_shared.ngetval++;

        sem_post(g_shared.mutex);
        sem_post(g_shared.nempty);

        *(static_cast<int*>(arg)) += 1;

    } while (true);

    return (NULL);
}


// todo: occasionally deadlock happends. cannot get mutex in consume threads
