#include <phinix/syscall.h>
#include <phinix/stdlib.h>
#include <phinix/signal.h>

int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        return -1;
    }

    return kill(atoi(argv[1]), SIGTERM);
}
