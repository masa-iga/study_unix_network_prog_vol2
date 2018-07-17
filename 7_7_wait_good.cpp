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
const int kNitems = 100 * 1000;

void* produce(void* arg);
void* consume(void* arg);

//struct
//{
//    pthread_mutex_t mutex;
//    int buff[kMaxNitems];
//    int nput;
//    int nval;
//} shared =
//{
//    PTHREAD_MUTEX_INITIALIZER
//};

int g_buff[kMaxNitems];

struct
{
    pthread_mutex_t mutex;
    int nput;
    int nval;
} put =
{
    PTHREAD_MUTEX_INITIALIZER
};

struct
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int nready;
} nready =
{
    PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER
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


void* produce(void* arg)
{
    while (true)
    {
        {
            pthread_mutex_lock(&put.mutex);

            if (put.nput >= kNitems)
            {
                pthread_mutex_unlock(&put.mutex);
                return (NULL);
            }

            g_buff[put.nput] = put.nval;
            put.nput++;
            put.nval++;

            pthread_mutex_unlock(&put.mutex);
        }

        {
            pthread_mutex_lock(&nready.mutex);

            if (nready.nready == 0)
                pthread_cond_signal(&nready.cond);

            nready.nready++;

            pthread_mutex_unlock(&nready.mutex);

            *(reinterpret_cast<int*>(arg)) += 1;
        }
    }
}


void* consume(void* arg)
{
    for (int i = 0; i < kNitems; i++)
    {
        {
            pthread_mutex_lock(&nready.mutex);

            while (nready.nready == 0) /* must be "while(cond)" to avoid spurious wake up */
                pthread_cond_wait(&nready.cond, &nready.mutex);

            nready.nready--;

            pthread_mutex_unlock(&nready.mutex);
        }

        if (g_buff[i] != i)
            VPRINTF("g_buff[%d] = %d\n", i, g_buff[i]);
    }

    return (NULL);
}



