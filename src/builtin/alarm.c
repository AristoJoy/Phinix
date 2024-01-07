#include <phinix/syscall.h>
#include <phinix/stdio.h>
#include <phinix/signal.h>

// 信号处理器
static int signal_handler(int sig)
{
    printf("pid %d when %d signal %d happened \a\n", getpid(), time(), sig);
    signal(SIGALRM, (int)signal_handler);
    alarm(2);
}

int main(int argc, char const *argv[])
{
    signal(SIGALRM, (int)signal_handler);
    alarm(2);
    while (true)
    {
        printf("pid %d when %d sleeping 1 second\n", getpid(), time());
        sleep(1000);
    }
    
    return 0;
}
