#include <phinix/device.h>
#include <phinix/string.h>
#include <phinix/task.h>
#include <phinix/assert.h>
#include <phinix/debug.h>
#include <phinix/arena.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define DEVICE_NR 64 // 设备数量

static device_t devices[DEVICE_NR]; // 设备数组

static device_t *get_null_device()
{
    for (size_t i = 1; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        if (device->type == DEV_NULL)
        {
            return device;
        }
    }
    panic("no more devices!!!");
}

// 安装设备
dev_t device_install(int type, int subtype, void *ptr, char *name, dev_t parent, void *ioctl, void *read, void *write)
{
    device_t *device = get_null_device();
    device->ptr = ptr;
    device->parent = parent;
    device->type = type;
    device->subtype = subtype;
    strncpy(device->name, name, NAMELEN);
    device->ioctl = ioctl;
    device->read = read;
    device->write = write;
    return device->dev;
}

// 根据子类型查找设备
device_t *device_find(int subtype, idx_t idx)
{
    idx_t nr = 0;
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        if (device->subtype != subtype)
        {
            continue;
        }
        if (nr == idx)
        {
            return device;
        }
        nr++;
    }

    return NULL;
}

// 根据设备号查找设备
device_t *device_get(dev_t dev)
{
    assert(dev < DEVICE_NR);
    device_t *device = &devices[dev];
    assert(device->type != DEV_NULL);
    return device;
}

// 执行块设备请求
void do_request(request_t *req)
{
    switch (req->type)
    {
    case REQ_READ:
        device_read(req->dev, req->buf, req->count, req->idx, req->flags);
        break;
    case REQ_WRITE:
        device_write(req->dev, req->buf, req->count, req->idx, req->flags);
        break;
    default:
        panic("req type %d unknown!!!", req->type);
        break;
    }
}

// 块设备请求
void device_request(dev_t dev, void *buf, u8 count, idx_t idx, int flags, u32 type)
{
    device_t *device = device_get(dev);
    assert(device->type == DEV_BLOCK); // 是块设备
    idx_t offset = idx + device_ioctl(device->dev, DEV_CMD_SECTOR_START, 0, 0);
    if (device->parent)
    {
        device = device_get(device->parent);
    }

    request_t *req = kmalloc(sizeof(request_t));

    req->dev = dev;
    req->buf = buf;
    req->count = count;
    req->idx = offset;
    req->flags = flags;
    req->type = type;
    req->task = NULL;

    // 判断列表是否为空
    bool empty = list_empty(&device->request_list);

    list_push(&device->request_list, &req->node);

    // 如果链表不为空
    if (!empty)
    {
        req->task = running_task();
        task_block(req->task, NULL, TASK_BLOCKED);
    }

    do_request(req);

    list_remove(&req->node);

    kfree(req);

    if (!list_empty(&device->request_list))
    {
        // 先来先服务
        request_t *next_req = element_entry(request_t, node, device->request_list.tail.prev);
        assert(next_req->task->magic == PHINIX_MAGIC);
        task_unblock(next_req->task);
    }
}
// 设备控制
int device_ioctl(dev_t dev, int cmd, void *args, int flags)
{
    device_t *device = device_get(dev);
    if (device->ioctl)
    {
        return device->ioctl(device->ptr, cmd, args, flags);
    }
    LOGK("ioctl of device %d not implemented!!!\n", dev);
    return EOF;
}
// 读设备
int device_read(dev_t dev, void *buf, size_t count, idx_t idx, int flags)
{
    device_t *device = device_get(dev);
    if (device->read)
    {
        return device->read(device->ptr, buf, count, idx, flags);
    }
    LOGK("read of device %d not implemented!!!\n", dev);
    return EOF;
}
// 写设备
int device_write(dev_t dev, void *buf, size_t count, idx_t idx, int flags)
{
    device_t *device = device_get(dev);
    if (device->write)
    {
        return device->write(device->ptr, buf, count, idx, flags);
    }
    LOGK("write of device %d not implemented!!!\n", dev);
    return EOF;
}

// 设备初始化
void device_init()
{
    for (size_t i = 0; i < DEVICE_NR; i++)
    {
        device_t *device = &devices[i];
        strcpy((char *)device->name, "null");
        device->type = DEV_NULL;
        device->subtype = DEV_NULL;
        device->dev = i;
        device->parent = 0;
        device->ioctl = NULL;
        device->read = NULL;
        device->write = NULL;
        list_init(&device->request_list);
    }
}
