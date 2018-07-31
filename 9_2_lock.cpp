#include <cstdio>
#include <sys/types.h> // for getpid()
#include <unistd.h> // for getpid()
#include <sys/stat.h> // for open()
#include <fcntl.h> // for open()
#include <string.h> // for strlen()

#define NO_LOCK (0)

#define VPRINTF(...) \
    do { \
        printf("%s (L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

void my_lock(int fd);
void my_unlock(int fd);

const char* kSeqFile = "seqno";
const size_t kMaxLine = 10;


int main(int argc, char **argv)
{

    pid_t pid = getpid();

    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IWOTH;
    int fd = open(kSeqFile, O_RDWR, fmode);

    for (int i = 0; i < 20; i++)
    {
        my_lock(fd);

        lseek(fd, 0L, SEEK_SET); // move to the head

        char line[kMaxLine + 1];
        ssize_t n = read(fd, line, kMaxLine);
        line[n] = '\0'; // Null termination for sscanf()

        long seqno;
        n = sscanf(line, "%ld\n", &seqno);

        VPRINTF("%s: pid = %ld, seq# = %ld\n", argv[0], static_cast<long>(pid), seqno);

        seqno++;

        snprintf(line, sizeof(line), "%ld\n", seqno);
        lseek(fd, 0L, SEEK_SET); // move to the head before writing
        write(fd, line, strlen(line));

        my_unlock(fd);
    }

    return 0;
}


void my_lock(int fd)
{
#if NO_LOCK
    return ;
#endif // NO_LOCK
    struct flock lock;

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    fcntl(fd, F_SETLKW, &lock);
}


void my_unlock(int fd)
{
#if NO_LOCK
    return ;
#endif // NO_LOCK
    struct flock lock;

    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    fcntl(fd, F_SETLK, &lock);
}
