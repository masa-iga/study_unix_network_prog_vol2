#include <cstdio>
#include <pthread.h>
#include <unistd.h>

#define VPRINTF(...) \
    do { \
        fprintf(stdout, "%s (L%d): ", __func__, __LINE__); \
        fprintf(stdout, __VA_ARGS__); \
    } while(false)

void *thread1(void *arg);
void *thread2(void *arg);

pthread_rwlock_t g_rwlock = PTHREAD_RWLOCK_INITIALIZER;
pthread_t g_tid1, g_tid2;


int main()
{
    pthread_rwlock_init(&g_rwlock, NULL);
    pthread_setconcurrency(2);

    pthread_create(&g_tid1, NULL /* attr */, thread1, NULL /* arg */);
    sleep(1);
    pthread_create(&g_tid2, NULL /* attr */, thread2, NULL /* arg */);

    void *status;
    pthread_join(g_tid2, &status);

    if (status != PTHREAD_CANCELED)
    {
        VPRINTF("thread2 status = %p\n", status);
    }

#if 0 // these members can't be found
    VPRINTF("rw_refcount = %d, rw_nwaitreaders = %d, rw_nwaitwriters = %d\n",
            g_rwlock.rw_refcount,
            g_rwlock.rw_nwaitreaders,
            g_rwlock.rw_nwaitwriters);
#endif

    pthread_rwlock_destroy(&g_rwlock);

    return 0;
}


void *thread1(void *arg)
{
    pthread_rwlock_rdlock(&g_rwlock);
    VPRINTF("got a read lock\n");

    sleep(3);

    VPRINTF("canceled the thread 2\n");
    pthread_cancel(g_tid2);

    sleep(3);

    pthread_rwlock_unlock(&g_rwlock);
    VPRINTF("unlocked a read lock\n");

    return (NULL);
}


void *thread2(void *arg)
{
    VPRINTF("trying to obtain a write lock\n");

    pthread_rwlock_wrlock(&g_rwlock);

    VPRINTF("got a write lock\n");

    sleep(1);

    pthread_rwlock_unlock(&g_rwlock);
    VPRINTF("unlocked a write lock\n");

    return (NULL);
}
