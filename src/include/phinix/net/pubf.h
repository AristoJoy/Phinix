#ifndef PHINIX_NET_PUBF_H
#define PHINIX_NET_PUBF_H

#include <phinix/types.h>
#include <phinix/list.h>
#include <phinix/net/eth.h>

// 缓冲包
typedef struct pbuf_t
{
    list_node_t node; // 列表节点
    size_t length;    // 载荷长度
    u32 count;        // 引用计数
    union
    {
        u8 payload[0]; // 载荷
        eth_t eth[0];  // 以太网帧
    };

} pbuf_t;

// 获取缓存包
pbuf_t *pbuf_get();
// 释放缓冲包
void pbuf_put(pbuf_t *pbuf);

#endif