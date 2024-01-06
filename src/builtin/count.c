#include <phinix/syscall.h>
#include <phinix/stdio.h>

int main(int argc, char const *argv[])
{
    int counter = 1;
    while (counter)
    {
        printf("hello phinix %d\a\n", counter++);
        sleep(1000);
    }
    
    return 0;
}
