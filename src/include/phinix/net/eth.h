#ifndef PHINIX_NET_ETH_H
#define PHINIX_NET_ETH_H

#include <phinix/net/types.h>
#include <phinix/net/arp.h>

#define ETH_FCS_LEN 4

#define ETH_BROADCAST "\xff\xff\xff\xff\xff\xff"
#define ETH_ANY "\x00\x00\x00\x00\x00\x00"

enum
{
    ETH_TYPE_IP = 0x0800,   // IPv4 协议
    ETH_TYPE_ARP = 0x0806,  // ARP 协议
    ETH_TYPE_IPV6 = 0x86DD, // IPv6 协议
};

// 以太网帧
typedef struct eth_t
{
    eth_addr_t dst; // 目标地址
    eth_addr_t src; // 源地址
    u16 type;       // 类型
    union
    {
        u8 payload[0]; // 载荷
        arp_t arp[0];  // arp 包
    };

} _packed eth_t;

// 接收以太网帧
err_t eth_input(netif_t *netif, pbuf_t *pbuf);
// 发送以太网帧
err_t eth_output(netif_t *netif, pbuf_t *pbuf, eth_addr_t dst, u16 type, u32 len);

#endif