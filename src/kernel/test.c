#include <phinix/types.h>
#include <phinix/cpu.h>
#include <phinix/printk.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/errno.h>
#include <phinix/string.h>
#include <phinix/net.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)
err_t sys_test()
{
    // 发送测试数据包
    ip_addr_t addr;
    assert(inet_aton("192.168.31.2", addr) == EOK);

    pbuf_t *pbuf = pbuf_get();
    netif_t *netif = netif_route(addr);

    ip_t *ip = pbuf->eth->ip;
    u16 len = 128 - sizeof(ip_t) - sizeof(eth_t);
    memset(ip->payload, 'T', len);
    ip_output(netif, pbuf, addr, 254, len);
}
