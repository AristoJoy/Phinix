#ifdef PHINIX
#include <phinix/types.h>
#include <phinix/stdio.h>
#include <phinix/syscall.h>
#include <phinix/string.h>
#else
#include <stdio.h>
#include <string.h>
#endif

int main(int argc, char const *argv[])
{
    for (size_t i = 1; i < argc; i++)
    {
        printf(argv[i]);
        if (i < argc - 1)
        {
            printf(" ");
        }
    }
    printf("\n");
    return 0;
}