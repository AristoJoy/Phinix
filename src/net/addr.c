#include <phinix/net/addr.h>
#include <phinix/string.h>
#include <phinix/stdlib.h>
#include <phinix/errno.h>

// MAC 地址拷贝
void eth_addr_copy(eth_addr_t dst, eth_addr_t src)
{
    memcpy(dst, src, ETH_ADDR_LEN);
}

// IP 地址拷贝
void ip_addr_copy(ip_addr_t dst, ip_addr_t src)
{
    *(u32 *)dst = *(u32 *)src;
}

// 字符串转换 IP 地址
err_t inet_aton(const char *str, ip_addr_t addr)
{
    const char *ptr = str;

    u8 parts[4];

    for (size_t i = 0; i < 4 && *ptr != '\0'; i++, ptr++)
    {
        int value = 0;
        int k = 0;
        for(; true; ptr++, k++)
        {
            if (*ptr == '.' || *ptr == '\0')
            {
                break;
            }
            if (!isdigit(*ptr))
            {
                return -EADDR;
            }
            value = value * 10 + ((*ptr) - '0');
            
        }
        if (k == 0 || value < 0 || value > 255)
        {
            return -EADDR;
        }
        parts[i] = value;
    }
    ip_addr_copy(addr, parts);
    return EOK;
}