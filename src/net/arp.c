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

// arp 响应
static err_t arp_reply(netif_t *netif, pbuf_t *pbuf)
{
    arp_t *arp = pbuf->eth->arp;

    LOGK("ARP Request from %r\n", arp->ipsrc);

    arp->opcode = htons(ARP_OP_REPLY);

    eth_addr_copy(arp->hwdst, arp->hwsrc);
    ip_addr_copy(arp->ipdst, arp->ipsrc);

    eth_addr_copy(arp->hwsrc, netif->hwaddr);
    ip_addr_copy(arp->ipsrc, netif->ipaddr);

    pbuf->count++;
    return eth_output(netif, pbuf, arp->hwdst, ETH_TYPE_ARP, sizeof(arp_t));
}

err_t arp_input(netif_t *netif, pbuf_t *pbuf)
{
    arp_t *arp = pbuf->eth->arp;
    // 只支持 以太网
    if (ntohs(arp->hwtype) != ARP_HARDWARE_ETH)
    {
        return -EPROTO;
    }

    // 只支持 IP
    if (ntohs(arp->proto) != ARP_PROTOCAL_IP)
    {
        return -EPROTO;
    }
    // 如果请求的目的地址不是本机 ip 则忽略
    if (!ip_addr_cmp(netif->ipaddr, arp->ipdst))
    {
        return -EPROTO;
    }

    u16 type = ntohs(arp->opcode);
    switch (type)
    {
    case ARP_OP_REQUEST:
        return arp_reply(netif, pbuf);
    case ARP_OP_REPLY:
        LOGK("arp reply %r -> %m\n", arp->ipsrc, arp->hwsrc);
        break;

    default:
        return -EPROTO;
    }
    return EOK;
}

// arp协议初始化
void arp_init()
{
}