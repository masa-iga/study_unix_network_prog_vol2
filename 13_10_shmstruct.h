#ifndef __SHM_STRUCT_H__

#define __SHM_STRUCT_H__
#include <fcntl.h> // sem_open()
#include <sys/stat.h> // sem_open()
#include <semaphore.h> // sem_open()

#define MESGSIZE    256
#define NMESG        16

struct shmStruct
{
    int nput;
    long noverflow;
    long msgoff[NMESG];
    char msgdata[NMESG * MESGSIZE];
};

struct semStruct
{
    sem_t* mutex;
    sem_t* nempty;
    sem_t* nstored;
    sem_t* noverflowmutex;
};


static const char* kPathShm = "/tmp/13_11_shm";
static const char* kPathSemMutex = "/tmp/13_11_sem_mutex";
static const char* kPathSemEmpty = "/tmp/13_11_sem_empty";
static const char* kPathSemStore = "/tmp/13_11_sem_store";
static const char* kPathSemOverflow = "/tmp/13_11_sem_overflow";

#endif // __SHM_STRUCT_H__
