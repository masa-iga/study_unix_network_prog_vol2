#include <cstdio>
#include <sys/types.h> // for open()
#include <sys/stat.h> // for open()
#include <fcntl.h> // for open()
#include <errno.h>
#include "../study_unix_network_prog_vol1/util.h"

const char* kPathPidfile = "pidfile";
const int kMaxLine = 32;

#define write_lock(fd, offset, whence, len) \
    lock_reg(fd, F_SETLK, F_WRLCK, offset, whence, len)

int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len);

int main(int argc, char* argv[])
{
    const mode_t fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    const int pidfd = open(kPathPidfile, O_RDWR | O_CREAT, fmode);

    if (write_lock(pidfd, 0, SEEK_SET, 0) < 0)
    { /* cannot get the lock */
        if (errno == EACCES || errno == EAGAIN)
        {
            err_quit("unable to lock %s, is %s already running?",
                    kPathPidfile, argv[0]);
            exit(1);
        }
        else
        {
            err_sys("unable to lock %s", kPathPidfile);
        }
    }

    /* can get the lock */
    char line[kMaxLine];
    snprintf(line, sizeof(line), "%ld\n", static_cast<long>(getpid()));
    ftruncate(pidfd, 0);
    write(pidfd, line, strlen(line));

    /* run daemon */
    printf("run daemon...\n");

    pause();

    return 0;
}


int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;
    lock.l_start = offset;
    lock.l_whence = whence;
    lock.l_len = len;

    return (fcntl(fd, cmd, &lock));
}
