#include <phinix/net.h>
#include <phinix/list.h>
#include <phinix/arena.h>
#include <phinix/syscall.h>
#include <phinix/string.h>
#include <phinix/task.h>
#include <phinix/debug.h>
#include <phinix/assert.h>
#include <phinix/errno.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

// 接收ip包
err_t ip_input(netif_t *netif, pbuf_t *pbuf)
{
    ip_t *ip = pbuf->eth->ip;

    // 只支持 IPv4
    if (ip->version != IP_VERSION_4)
    {
        LOGK("IP version %d error\n", ip->version);
        return -EPROTO;
    }
    // TTL 耗尽
    if (!ip->ttl)
    {
        return -ETIME;
    }

    // 不支持该选项
    if (ip->header != sizeof(ip_t) / 4)
    {
        LOGK("IP option error\n");
        return -EOPTION;
    }

    // 不支持分片
    if (ip->flags & IP_FLAT_NOLAST)
    {
        LOGK("IP fragment error\n");
        return -EFRAG;
    }

    // 不是本机 IP
    if (!ip_addr_cmp(ip->dst, netif->ipaddr))
    {
        return -EADDR;
    }

    ip->length = ntohs(ip->length);
    ip->id = ntohs(ip->id);

    switch (ip->proto)
    {
    case IP_PROTOCOL_TCP:
        LOGK("IP:TCP received\n");
        break;
    case IP_PROTOCOL_UDP:
        LOGK("IP:UDP received\n");
        break;
    case IP_PROTOCOL_ICMP:
        return icmp_input(netif, pbuf); // ICMP 输入
    default:
        LOGK("IP {%X}: %r -> %r \n", ip->proto, ip->src, ip->dst);
        return -EPROTO;
    }
    return EOK;
}

// 发送ip包
err_t ip_output(netif_t *netif, pbuf_t *pbuf, ip_addr_t dst, u8 proto, u16 len)
{
    ip_t *ip = pbuf->eth->ip;

    ip->header = 5;
    ip->version = 4;

    ip->flags = 0;
    ip->offset0 = 0;
    ip->offset1 = 0;

    ip->tos = 0;
    ip->id = 0;

    ip->ttl = IP_TTL;
    ip->proto = proto;

    u16 length = sizeof(ip_t) + len;
    ip->length = htons(length);

    // 一定要先对 dst 复制，因为dst指针可能是下面的src
    ip_addr_copy(ip->dst, dst);
    ip_addr_copy(ip->src, netif->ipaddr);

    ip->chksum = 0;
    ip->chksum = ip_chksum(ip, sizeof(ip_t));

    return arp_eth_output(netif, pbuf, ip->dst, ETH_TYPE_IP, length);
}

// 初始化ip协议
void ip_init()
{
}
