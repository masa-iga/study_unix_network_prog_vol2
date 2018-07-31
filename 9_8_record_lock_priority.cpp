#include <cstdio>
#include <sys/types.h> // for open()
#include <sys/stat.h> // for open()
#include <fcntl.h> // for open()
#include <unistd.h> // for fork()
#include <cstdlib> // for exit()
#include <chrono>

#define DO_READ_LOCK (0)

#define read_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLK, F_RDLCK, offset, whence, len)

#define readw_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLKW, F_RDLCK, offset, whence, len)

#define write_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)

#define writew_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLKW, F_WRLCK, offset, whence, len)

#define un_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLK, F_UNLCK, offset, whence, len)

#define VPRINTF(...) \
    do { \
        printf("%s (L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

const char* fname = "test1.data";

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len);


struct TimeCounter
{
    TimeCounter()
        : base_(std::chrono::system_clock::now()) { }

    std::chrono::time_point<std::chrono::system_clock> base_;
    double getTimeDouble();
};

double TimeCounter::getTimeDouble()
{
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = now - base_;
    return diff.count();
}


int main()
{
    TimeCounter timeCounter;

    mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    int fd = open(fname, O_RDWR | O_CREAT, fmode);

#if DO_READ_LOCK
    read_lock(fd, 0, SEEK_SET, 0);
    VPRINTF("%lf: parent has read lock\n", timeCounter.getTimeDouble());
#else
    write_lock(fd, 0, SEEK_SET, 0);
    VPRINTF("%lf: parent has write lock\n", timeCounter.getTimeDouble());
#endif // DO_READ_LOCK

    if (fork() == 0) /* first child process */
    {
        sleep(1);

        VPRINTF("%lf: first child tries to obtain write lock\n", timeCounter.getTimeDouble());
        writew_lock(fd, 0, SEEK_SET, 0);
        VPRINTF("%lf: first child obtains write lock\n", timeCounter.getTimeDouble());

        sleep(2);

        un_lock(fd, 0, SEEK_SET, 0);
        VPRINTF("%lf: first child releases write lock\n", timeCounter.getTimeDouble());

        exit(0);

    }

    if (fork() == 0) /* second child process */
    {
        sleep(3);

        VPRINTF("%lf: second child tries to obtain read lock\n", timeCounter.getTimeDouble());
        readw_lock(fd, 0, SEEK_SET, 0);
        VPRINTF("%lf: second child obtains read lock\n", timeCounter.getTimeDouble());

        sleep(4);

        un_lock(fd, 0, SEEK_SET, 0);
        VPRINTF("%lf: second child releases read lock\n", timeCounter.getTimeDouble());

        exit(0);

    }

    sleep(5);

    un_lock(fd, 0, SEEK_SET, 0);
    VPRINTF("%lf: parent releases read lock\n", timeCounter.getTimeDouble());

    exit(0);
}


int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;     /* F_RDLCK, F_WRLCK, F_UNLCK */
    lock.l_start = offset;
    lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
    lock.l_len = len;       /* size in bytes. (0 means EoF) */

    return (fcntl(fd, cmd, &lock)); /* return -1 if error happens */
}
