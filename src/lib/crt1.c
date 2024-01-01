#include <phinix/types.h>
#include <phinix/syscall.h>
#include <phinix/string.h>

int main(int argc, char **argv, char **envp);

// libc构造函数
weak void _init()
{

}

// libc析构函数
weak void _fini()
{

}

// 启动用户程序入口
int __libc_start_main(
    int (*main)(int argc, char **argv, char **envp),
    int argc, char **argv,
    void (*_init)(),
    void (*_fini)(),
    void (*ldso)(), // 动态链接器 
    void *stack_end)
{
    char **envp = argv + argc + 1;
    _init();
    int i = main(argc, argv, envp);
    _fini();
    exit(i);
}