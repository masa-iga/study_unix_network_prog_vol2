#include <cstdio>
#include <pthread.h>

#define MUTEX_INVALIDATED (0) // test whether mutex works fine

#define VPRINTF(...) \
    do { \
        printf("%s (%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false) \


const int kMaxNItems = 1000 * 1000;
const int kMaxNThreads = 100;

const int nitems = 100 * 1000;

void* produce(void*);
void* consume(void*);

struct {
    pthread_mutex_t mutex;
    int buff[kMaxNItems];
    int nput;
    int nval;
} shared = {
    PTHREAD_MUTEX_INITIALIZER
};


int main()
{
    const int nthreads = 8;

    int count[kMaxNThreads];
    pthread_t tid_produce[kMaxNThreads], tid_consume;

    pthread_setconcurrency(nthreads);

    for (int i = 0; i < nthreads; i++)
    {
        /* Launch all of producer threads */
        count[i] = 0;
        pthread_create(&tid_produce[i], NULL /* pthread_attr_t */, produce, &count[i] /* void* arg */);
    }

    for (int i = 0; i < nthreads; i++)
    {
        /* Wait until all of producer threads stop */
        pthread_join(tid_produce[i], NULL /* void** retval */);
        VPRINTF("thread %d: count %d\n", i, count[i]);
    }

    /* Launch consumer thread and wait for the end */
    pthread_create(&tid_consume, NULL, consume, NULL);
    pthread_join(tid_consume, NULL);

    return 0;
}


void* produce(void* arg)
{
    while (true)
    {
#if !MUTEX_INVALIDATED
        pthread_mutex_lock(&shared.mutex);
#endif // !MUTEX_INVALIDATED

        if (shared.nput >= nitems)
        {
            /* If buffer is full, it finishes */
#if !MUTEX_INVALIDATED
            pthread_mutex_unlock(&shared.mutex);
#endif // !MUTEX_INVALIDATED
            return (NULL);
        }

        shared.buff[shared.nput] = shared.nval;
        shared.nput++;
        shared.nval++;

#if !MUTEX_INVALIDATED
        pthread_mutex_unlock(&shared.mutex);
#endif // !MUTEX_INVALIDATED

        *((int*)arg) += 1;
    }
}


void* consume(void* arg)
{
    for (int i = 0; i < nitems; i++)
    {
        if (shared.buff[i] != i)
            VPRINTF("Error. buff[%d] = %d\n", i, shared.buff[i]);
    }

    return (NULL);
}
