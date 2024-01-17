#ifndef PHINIX_NET_ETH_H
#define PHINIX_NET_ETH_H

#include <phinix/net/types.h>

#define ETH_CRC_LEN 4

// 以太网帧
typedef struct eth_t
{
    eth_addr_t dst; // 目标地址
    eth_addr_t src; // 源地址
    u16 type; // 类型
    u8 payload[0]; // 载荷
} _packed eth_t;


#endif