#include <phinix/bitmap.h>
#include <phinix/string.h>
#include <phinix/phinix.h>
#include <phinix/assert.h>

// 初始化位图
void bitmap_init(bitmap_t *map, char *bits, u32 length, u32 offset)
{
    memset(bits, 0, length);
    bitmap_make(map, bits, length, offset);
}

// 构造位图
void bitmap_make(bitmap_t *map, char *bits, u32 length, u32 offset)
{
    map->bits = bits;
    map->length = length;
    map->offset = offset;
}

// 测试位图的某一位是否为1
bool bitmap_test(bitmap_t *map, u32 index)
{
    assert(index >= map->offset);

    // 得到位图的索引
    idx_t idx = index - map->offset;

    // 位图数组中的字符串
    u32 bytes = idx / 8;
    u8 bits = idx % 8;

    assert(bytes < map->length);

    // 返回那一位是否等于1
    return (map->bits[bytes] & (1 << bits));
}

// 设置位图的某位的值
void bitmap_set(bitmap_t *map, u32 index, bool value)
{
    // value必须是二值
    assert(value == 0 || value == 1);

    assert(index >= map->offset);

    // 得到位图的索引
    idx_t idx = index - map->offset;

    // 位图数组中的字符串
    u32 bytes = idx / 8;
    u8 bits = idx % 8;

    if (value)
    {
        // 置为1
        map->bits[bytes] |= (1 << bits);
    }
    else
    {
        // 置为0
        map->bits[bytes] &= ~(1 << bits);
    }
}

// 从位图中得到连续的count位
int bitmap_scan(bitmap_t *map, u32 count)
{
    int start = EOF;

    u32 bits_left = map->length * 8;
    u32 next_bit = 0;
    u32 counter = 0;

    // 从头开始找
    while (bits_left-- > 0)
    {
        // 如果下一位没有被占用，这计数器加1
        if (!(bitmap_test(map, map->offset + next_bit)))
        {
            counter++;
        }
        else
        {
            counter = 0;
        }

        next_bit++;

        if (counter == count)
        {
            start = next_bit - count;
            break;
        }
    }
    if (start == EOF)
    {
        return EOF;
    }

    // 否则将找到的位全部置为1
    bits_left = count;
    next_bit = start;
    while (bits_left-- > 0)
    {
        bitmap_set(map, map->offset + next_bit, true);
        next_bit++;
    }

    return start + map->offset;
}

#include <phinix/debug.h>

#define LOGK(fmt, args...) DEBUGK(fmt, ##args)

#define LEN 2
u8 buf[LEN];
bitmap_t map;

void bitmap_tests()
{
    bitmap_init(&map, buf, LEN, 0);
    for (size_t i = 0; i < 33; i++)
    {
        idx_t idx = bitmap_scan(&map, 1);
        if (idx == EOF)
        {
            LOGK("TEST FINNISH\n");
            break;
        }
        LOGK("%d\n", idx);
    }
    
}