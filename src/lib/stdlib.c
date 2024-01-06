#include <phinix/stdlib.h>

void delay(u32 count)
{
    while (count--);
    
}

void hang()
{
    while(true);
}

/**
 * @brief 将8进制bcd码转化为二进制码
 * 
 * @param value 
 * @return u8 
 */
u8 bcd_to_bin(u8 value)
{
    return (value & 0xf) + (value >> 4) * 10;
}

/**
 * @brief 将8进制二进制码转化为bcd码
 *
 * @param value
 * @return u8
 */
u8 bin_to_bcd(u8 value)
{
    return ((value / 10) << 4) + (value % 10);
}


u32 div_round_up(u32 num, u32 size)
{
    return (num + size - 1) / size;
}

int atoi(const char *str)
{
    if (str == NULL)
    {
        return 0;
    }

    int sign = 1;
    int result = 0;
    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    for(; *str; str++)
    {
        result = result * 10 + (*str - '0');
    }
    
    return result * sign;
}