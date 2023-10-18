#include <phinix/phinix.h>
#include <phinix/types.h>
#include <phinix/io.h>
#include <phinix/string.h>
#include <phinix/console.h>

// #define CRT_ADDR_REG 0x3d4
// #define CRT_DATA_REG 0x3d5

// #define CRT_CURSOR_H 0xe
// #define CRT_CURSOR_L 0xf

char message[] = "hello phinix!!!\n";
char buf[1024];

void kernel_init()
{
    // // 获取光标高8位
    // out_byte(CRT_ADDR_REG, CRT_CURSOR_H);
    // u16 pos = in_byte(CRT_DATA_REG) << 8;

    // // 获取光标低8位
    // out_byte(CRT_ADDR_REG, CRT_CURSOR_L);
    // pos |= in_byte(CRT_DATA_REG);
    // u8 data = in_byte(CRT_DATA_REG);

    // // 设置位置回0 0
    // out_byte(CRT_ADDR_REG, CRT_CURSOR_H);
    // out_byte(CRT_DATA_REG, 0);
    // out_byte(CRT_ADDR_REG, CRT_CURSOR_L);
    // out_byte(CRT_DATA_REG, 0);


    // int res;
    // res = strcmp(buf, message);
    // strcpy(buf, message);
    // res = strcmp(buf, message);
    // strcat(buf, message);
    // res = strcmp(buf, message);
    // res = strlen(message);
    // res = sizeof(message);

    // char *ptr = strchr(message, '!');
    // ptr = strrchr(message, '!');

    // memset(buf, 0, sizeof(buf));
    // res = memcmp(buf, message, sizeof(message));
    // memcpy(buf, message, sizeof(message));
    // res = memcmp(buf, message, sizeof(message));
    // ptr = memchr(buf, '!', sizeof(message));

    console_init();
    u32 count = 30;
    while (true)
    {
        console_write(message, sizeof(message) - 1);
    } 
    
    return;
}