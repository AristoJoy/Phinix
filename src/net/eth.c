#include <phinix/net.h>
#include <phinix/string.h>
#include <phinix/debug.h>
#include <phinix/errno.h>
#include <phinix/assert.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

// 接收以太网帧
err_t eth_input(netif_t *netif, pbuf_t *pbuf)
{
    eth_t *eth = (eth_t *)pbuf->eth;
    u16 type = ntohs(eth->type);

    switch (type)
    {
    case ETH_TYPE_IP:
        return ip_input(netif, pbuf); // IP 输入
    case ETH_TYPE_IPV6:
        // LOGK("ETH %m -> %m IPv6, %d\n", eth->src, eth->dst, pbuf->length);
        break;
    case ETH_TYPE_ARP:
        return arp_input(netif, pbuf); // ARP 输入
    default:
        LOGK("ETH %m -> %m UNKNOWN, %d\n", eth->src, eth->dst, pbuf->length);
        return -EPROTO;
    }
    return EOK;
}

// 发送以太网帧
err_t eth_output(netif_t *netif, pbuf_t *pbuf, eth_addr_t dst, u16 type, u32 len)
{
    pbuf->eth->type = htons(type);
    eth_addr_copy(pbuf->eth->dst, dst);
    eth_addr_copy(pbuf->eth->src, netif->hwaddr);
    pbuf->length = sizeof(eth_t) + len;
    
    netif_output(netif, pbuf);
    return EOK;
}

// 初始化以太网协议
void eth_init()
{
}
