#include <cstdio>
#include <unistd.h> // getopt()
#include <sys/mman.h> // shm_open()
#include <sys/stat.h> // mode def
#include <fcntl.h> // constant def
#include <cstdlib> // exit()

#define VPRINTF(...) \
    do { \
        printf("L%d %s(): ", __LINE__, __func__); \
        printf(__VA_ARGS__); \
    } while(false)



void create_13_2(const char* name, int length, int flags)
{

    VPRINTF("start\n");

    int fd;
    uint8_t *ptr;

    {
        const int fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

        fd = shm_open(name, flags, fmode);

        if (fd == -1)
        {
            VPRINTF("shm_open() error\n");
            exit(1);
        }

        VPRINTF("shm_oepn() succeeded. flags 0x%x  fmode 0x%x\n", flags, fmode);

        ftruncate(fd, length);

        ptr = reinterpret_cast<uint8_t*>(mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    }

}


void unlink_13_3(const char* name)
{
    VPRINTF("start\n");

    const int ret = shm_unlink(name);

    if (ret == -1)
    {
        VPRINTF("shm_unlink() error\n");
    }

}


void write_13_4(const char* name)
{
    VPRINTF("start\n");

    const int fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fd = shm_open(name, O_RDWR, fmode);

    if (fd == -1)
    {
        VPRINTF("shm_open() error\n");
        exit(1);
    }

    struct stat stat;
    fstat(fd, &stat);

    VPRINTF("mode 0x%x  uid %d  gid %d  st_size = %lld\n",
            stat.st_mode, stat.st_uid, stat.st_gid, stat.st_size);

    uint8_t* ptr = reinterpret_cast<uint8_t*>(mmap(NULL, stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));

    close(fd);

    /* write */
    for (int i = 0; i < stat.st_size; i++)
    {
        *ptr++ = i % 256;
    }

}


void read_13_5(const char* name)
{
    VPRINTF("start\n");

    const int fmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    int fd = shm_open(name, O_RDONLY, fmode);

    if (fd == -1)
    {
        VPRINTF("shm_open() error\n");
        exit(1);
    }

    struct stat stat;
    fstat(fd, &stat);

    VPRINTF("mode 0x%x  uid %d  gid %d  st_size = %lld\n",
            stat.st_mode, stat.st_uid, stat.st_gid, stat.st_size);

    uint8_t* ptr = reinterpret_cast<uint8_t*>(mmap(NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0));

    close(fd);

    for (int i = 0; i < stat.st_size; i++)
    {
        uint8_t c;
        if ((c = *ptr++) != (i % 256))
        {
            VPRINTF("error. ptr[%d] = %d\n", i, c);
            exit(0);
        }
    }

}


int main(int argc, char **argv)
{

    const char *optstr = "ecuwr";
    bool do_create = false, do_unlink = false, do_write = false, do_read = false;

    int c, flags = O_RDWR | O_CREAT;

    while ( (c = getopt(argc, argv, optstr)) != -1)
    {
        switch (c)
        {
        case 'e':
              flags |= O_EXCL; // return error if same name shared memory object exists
              break;

        case 'c':
            do_create = true;
            break;

        case 'u':
            do_unlink = true;
            break;

        case 'w':
            do_write = true;
            break;

        case 'r':
            do_read = true;
            break;
        }
    }

    if (optind != argc-1)
    {
        printf("usage: shmcreate [-e] [-c] [-u] [-w] [-r] <length>\n");
        exit(1);
    }

    const char* name = "/tmp/shmtest";
    const int length = atoi(argv[optind]);

    VPRINTF("name %s  length %d\n", name, length);

    if (do_create)
        create_13_2(name, length, flags);

    if (do_write)
        write_13_4(name);

    if (do_read)
        read_13_5(name);

    if (do_unlink)
        unlink_13_3(name);

    return 0;
}

