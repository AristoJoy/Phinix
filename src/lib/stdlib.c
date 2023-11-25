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
