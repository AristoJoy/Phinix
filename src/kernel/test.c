#include <phinix/types.h>
#include <phinix/cpu.h>
#include <phinix/printk.h>
#include <phinix/debug.h>
#include <phinix/errno.h>
#include <phinix/string.h>
#include <phinix/net.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)
err_t sys_test()
{
    // 发送测试数据包

    pbuf_t *pbuf = pbuf_get();
    netif_t *netif = netif_get();

    int len = 1500;
    memset(pbuf->eth->payload, 'A', len);
    eth_output(netif, pbuf, "\xff\xff\xff\xff\xff\x00", 0x9000, len);
}
