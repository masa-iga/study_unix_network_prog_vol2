#include <cstdio>
#include <cassert>
#include <pthread.h>

#define VPRINTF(...) \
    do { \
        printf("%s(L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

const int kMaxNitems = 1000 * 1000;
const int kMaxNthreads = 100;
const int kNthreads = 8;
const int kNitems = 1;

void* produce(void* arg);
void* consume(void* arg);
void consume_wait(int i);

struct
{
    pthread_mutex_t mutex;
    int buff[kMaxNitems];
    int nput;
    int nval;
} shared =
{
    PTHREAD_MUTEX_INITIALIZER
};


int main()
{

    int count[kMaxNthreads];
    pthread_t tid_produce[kMaxNthreads], tid_consume;

    /* All of producers plus one consumer threads */
    pthread_setconcurrency(kNthreads + 1);

    for (int i = 0; i < kNthreads; i++)
    {
        count[i] = 0;
        pthread_create(&tid_produce[i], NULL, produce, &count[i]);
    }

    pthread_create(&tid_consume, NULL, consume, NULL);

    for (int i = 0; i <kNthreads; i++)
    {
        pthread_join(tid_produce[i], NULL /* retval */);
        VPRINTF("count[%d] = %d\n", i, count[i]);
    }

    pthread_join(tid_consume, NULL /* retval */);

    return 0;
}


void* consume(void* arg)
{
    for (int i = 0; i < kNitems; i++)
    {
        consume_wait(i);

        if (shared.buff[i] != i)
        {
            VPRINTF("error. buff[%d] = %d\n", i, shared.buff[i]);
            assert(false);
        }
    }

    return (NULL);
}


void consume_wait(int i)
{
    while (true)
    {
        pthread_mutex_lock(&shared.mutex);

        if (i < shared.nput)
        {
            pthread_mutex_unlock(&shared.mutex);
            return;
        }

        pthread_mutex_unlock(&shared.mutex);
    }
}


void* produce(void* arg)
{
    while (true)
    {
        pthread_mutex_lock(&shared.mutex);

        if (shared.nput >= kNitems)
        {
            pthread_mutex_unlock(&shared.mutex);
            return (NULL);
        }

        shared.buff[shared.nput] = shared.nval;
        shared.nput++;
        shared.nval++;

        pthread_mutex_unlock(&shared.mutex);

        *reinterpret_cast<int*>(arg) += 1;
    }
}

