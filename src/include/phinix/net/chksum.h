#ifndef PHINIX_NET_CHKSUM_H
#define PHINIX_NET_CHKSUM_H

#include <phinix/types.h>

// 以太网校验和
u32 eth_fcs(void *data, int len);
// 校验
u16 chksum(void *data, int len);

#endif