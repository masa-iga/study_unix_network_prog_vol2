#include <sys/types.h> // for mkfifo()
#include <sys/stat.h> // for mkfifo()
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <stdarg.h>
#include <semaphore.h>
#include <errno.h>
#include <memory> // for std::uqnique_ptr<>
#include <unistd.h>

#define VPRINTF(...) \
    do { \
        printf("%s(L%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    } while (false)

/* This value is checked by each function that is passed a sem_t pointer, to make certain that the pointer really points to an initialized semaphore structure. This member is set to 0 when semaphore is closed. This technique, although not perfect, can help detect some programming errors. */
const uint32_t kSemMagic = 0x89674523;

/* FIFO based implementation */
class FifoBasedSemaphore
{
private:
    typedef struct
    {
        int sem_fd[2]; // [0]: RD only, [1]: WR only
        int sem_magic;
    } mysem_t;

public:
    FifoBasedSemaphore() : m_sem(nullptr) { };

    int sem_open(const char *pathname, int oflag, ...);
    int sem_close();
    int sem_unlink(const char *pathname);
    int sem_post();
    int sem_wait();

private:
    mysem_t *m_sem;
};


int FifoBasedSemaphore::sem_open(const char *pathname, int oflag, ...)
{
    if (m_sem != nullptr)
    { // already opened
        return -1;
    }

    unsigned int value;

    if (oflag & O_CREAT)
    {
        va_list ap;
        va_start(ap, oflag);

        const mode_t mode = va_arg(ap, int);
        value = va_arg(ap, unsigned int);

        va_end(ap);

        VPRINTF("mode: 0x%x\n", mode);
        VPRINTF("value: 0x%x\n", value);

        if (mkfifo(pathname, mode) < 0)
        {
            if (errno == EEXIST && (oflag & O_EXCL) == 0)
            {
                // already exists; OKay
                oflag &= ~O_CREAT;
            }
            else
            {
                VPRINTF("mkfiro() failed.\n");
                return -1;
            }
        }
    }


    m_sem = new mysem_t;

    if (m_sem == nullptr)
    {
        return -1;
    }

    m_sem->sem_fd[0] = m_sem->sem_fd[1] = -1;

    if ((m_sem->sem_fd[0] = open(pathname, O_RDONLY | O_NONBLOCK)) < 0)
        goto error;

    if ((m_sem->sem_fd[1] = open(pathname, O_WRONLY | O_NONBLOCK)) < 0)
        goto error;

    /* turn off nonblocking for sem_fd[0] */
    int flags;

    if ((flags = fcntl(m_sem->sem_fd[0], F_GETFL, 0)) < 0)
        goto error;

    flags &= ~O_NONBLOCK;

    if (fcntl(m_sem->sem_fd[0], F_SETFL, flags) < 0)
        goto error;

    if (oflag & O_CREAT)
    { /* initialize semaphores */
        for (int i = 0; i < value; i++)
        {
            char c;
            if (write(m_sem->sem_fd[i], &c, 1) != 1)
            {
                goto error;
            }
        }
    }

    m_sem->sem_magic = kSemMagic;
    return 0;

error:
    int save_errno = errno;

    if (oflag & O_CREAT)
    {
        unlink(pathname);
    }

    close(m_sem->sem_fd[0]);
    close(m_sem->sem_fd[1]);

    errno = save_errno;

    return -1;
}


int FifoBasedSemaphore::sem_close()
{
    if (m_sem->sem_magic != kSemMagic)
    {
        errno = EINVAL;
        return (-1);
    }

    m_sem->sem_magic = 0;

    if (close(m_sem->sem_fd[0]) == -1 || close(m_sem->sem_fd[1]) == -1)
    {
        free(m_sem);
        return (-1);
    }

    free(m_sem);
    return (0);
}


int FifoBasedSemaphore::sem_unlink(const char *pathname)
{
    return (unlink(pathname));
}


int FifoBasedSemaphore::sem_post()
{
    if (m_sem->sem_magic != kSemMagic)
    {
        errno = EINVAL;
        return (-1);
    }

    char c;

    if (write(m_sem->sem_fd[1], &c, 1) == 1)
    {
        return (0);
    }

    return (-1);
}


int FifoBasedSemaphore::sem_wait()
{
    if (m_sem->sem_magic != kSemMagic)
    {
        errno = EINVAL;
        return (-1);
    }

    char c;

    if (write(m_sem->sem_fd[1], &c, 1) == 1)
    {
        return (0);
    }

    return (-1);
}


int main()
{
    return 0;
}
