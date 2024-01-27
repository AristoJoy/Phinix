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

// ICMP 查询响应
static err_t icmp_echo_reply(netif_t *netif, pbuf_t *pbuf)
{
    ip_t *ip = pbuf->eth->ip;
    if (ip_addr_isbroadcast(ip->dst, netif->netmask) || ip_addr_ismulticast(ip->dst))
    {
        return -EPROTO;
    }
    if (!ip_addr_cmp(ip->dst, netif->ipaddr))
    {
        return -EPROTO;
    }
    
    icmp_t *icmp = ip->icmp;
    icmp->type = ICMP_ER;
    icmp->chksum = 0;

    u16 len = ip->length - sizeof(ip_t);
    icmp->chksum = ip_chksum(icmp, len);

    LOGK("IP ICMP ECHO REPLY: %r -> %r\n", ip->dst, ip->src);

    pbuf->count++;
    return ip_output(netif, pbuf, ip->src, IP_PROTOCOL_ICMP, len);
}

// 读取icmp包
err_t icmp_input(netif_t *netif, pbuf_t *pbuf)
{
    ip_t *ip = pbuf->eth->ip;
    icmp_t *icmp = ip->icmp;
    switch (icmp->type)
    {
    case ICMP_ER:
        LOGK("IP ICMP REPLY: %r -> %r \n", ip->src, ip->dst);
        break;
    case ICMP_ECHO:
        return icmp_echo_reply(netif, pbuf);
    default:
        LOGK("IP ICMP other: %r -> %r \n", ip->src, ip->dst);
        break;
    }
    return EOK;
}

// icmp echo
err_t icmp_echo(netif_t *netif, pbuf_t *pbuf, ip_addr_t dst)
{
    ip_t *ip = pbuf->eth->ip;
    icmp_echo_t *echo = ip->echo;

    echo->code = 0;
    echo->type = ICMP_ECHO;
    echo->id = 1;
    echo->seq = 1;

    char message[] = "Phinix icmp echo 123456789 asdfghjkl;'";

    strcpy(echo->payload, message);

    u32 len = sizeof(icmp_echo_t) + sizeof(message);

    echo->chksum = 0;
    echo->chksum = ip_chksum(echo, len);

    LOGK("IP ICMP ECHO: %r \n", dst);
    return ip_output(netif, pbuf, dst, IP_PROTOCOL_ICMP, len);
}

// icmp 协议初始化
void icmp_init()
{
}
