#ifdef PHINIX
#include <phinix/types.h>
#include <phinix/stdio.h>
#include <phinix/syscall.h>
#include <phinix/string.h>
#include <phinix/fs.h>
#else
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#endif

#define BUFLEN 1024

static char buf[BUFLEN];

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        return EOF;
    }
    int fd = open(argv[1], O_RDONLY, 0);
    if (fd == EOF)
    {
        printf("file %s not exists.\n", argv[1]);
        return;
    }

    while (1)
    {
        int len = read(fd, buf, BUFLEN);
        if (len == EOF)
        {
            break;
        }
        write(1, buf, len);
    }
    close(fd);
    return 0;
}
