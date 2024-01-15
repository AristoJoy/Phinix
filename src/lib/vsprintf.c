#include <phinix/stdarg.h>
#include <phinix/string.h>
#include <phinix/assert.h>

#define ZERO_PAD 0x01 // 填充0
#define SIGN 0x02     // signed long
#define PLUS 0x04     // 显示符号位
#define SPACE 0x08    // 如果不显示符号位，则显示空格
#define LEFT 0x10     // 左对齐
#define SPECIAL 0x20  // 特殊进制
#define SMALL 0x40    // 小写字母
#define DOUBLE 0x80   // 浮点数

#define is_digit(c) ((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s)
{
    int i = 0;
    while (is_digit(**s))
    {
        i = i * 10 + *((*s)++) - '0';
    }
    return i;
}

/**
 * 将整数转化为指定进制的字符串
 * str - 输出字符串指针
 * num - 整数
 * base - 进制基数
 * size - 字符串长度
 * precision - 数字长度(精度)
 * flags - 标志位
 */
static char *number(char *str, u32 *num, int base, int size, int precision, int flags)
{
    char pad, sign, tmp[36];
    const char *chm = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int i;
    int index;
    char *ptr = str;

    // 如果flags要用小写字幕，这定义小写字母表
    if (flags & SMALL)
    {
        chm = "0123456789abcdefghijklmnopqrstuvwxyz";
    }

    // 如果flags定义左对齐，则屏蔽0填充
    if (flags & LEFT)
    {
        flags &= ~ZERO_PAD;
    }
    // 只处理2~32之间基数的数字
    if (base < 2 || base > 32)
    {
        return 0;
    }

    // 如果指定了0填充，否则空格填充
    pad = (flags & ZERO_PAD) ? '0' : ' ';

    // 如果指定了有符号数，且数值小于0，则输出-，且将值转化为正数
    if (flags & DOUBLE && (*(double *)(num)) < 0)
    {
        sign = '-';
        *(double *)(num) = -(*(double *)(num));
    }
    else if ((flags & SIGN) && !(flags & DOUBLE) && ((int)(*num)) < 0)
    {
        sign = '-';
        num = -(int)(*num);
    }
    // 如果指定了输出符号位，则输出+；否则指定了空格，则输出空格到符号位；否则置0
    else
    {
        sign = (flags & PLUS) ? '+' : ((flags & SPACE) ? ' ' : 0);
    }

    // 如果有符号位，则输出位置占1位
    if (sign)
    {
        size--;
    }

    // 如flags输出特殊格式，16进制宽度减2(0x)，8进制宽度减1（0)
    if (flags & SPECIAL)
    {
        if (base == 16)
        {
            size -= 2;
        }
        else if (base == 8)
        {
            size--;
        }
    }

    // 下面数值转换为字符串倒序保存到数组里
    i = 0;
    if (flags & DOUBLE)
    {
        u32 ival = (u32)(*(double *)num);
        u32 fval = (u32)(((*(double *)num) - ival) * 1000000);
        do
        {
            index = (fval) % base;
            (fval) /= base;
            tmp[i++] = chm[index];
        } while (fval);
        tmp[i++] = '.';

        do
        {
            index = (ival) % base;
            (ival) /= base;
            tmp[i++] = chm[index];
        } while (ival);
    }
    else if ((*num) == 0)
    {
        tmp[i++] = '0';
    }
    else
    {
        while ((*num) != 0)
        {
            index = (*num) % base;
            (*num) /= base;
            tmp[i++] = chm[index];
        }
    }

    // 如果数值长度大于精度，则扩充精度
    if (i > precision)
    {
        precision = i;
    }

    // 宽度减去精度值
    size -= precision;

    // 从这里真正开始形成所需要的转换结果，并暂时放在字符串 str 中

    // 先考虑填充，如果没有左对齐和0填充，则首先填放剩余宽度值指出的空格数
    if (!(flags & (LEFT + ZERO_PAD)))
    {
        while (size-- > 0)
        {
            *ptr++ = ' ';
        }
    }

    // 如果有符号位，则输出
    if (sign)
    {
        *ptr++ = sign;
    }

    // 如果是特殊格式数字，8进制输出0,16进制输出0x
    if (flags & SPECIAL)
    {
        if (base == 8)
        {
            *ptr++ = '0';
        }
        else if (base == 16)
        {
            *ptr++ = '0';
            *ptr++ = chm[33];
        }
    }

    // 如果不是左对齐
    if (!(flags & LEFT))
    {
        // 如果宽度大于0，需要填充pad字符，这里只会有一种情况，pad是'0'
        while (size-- > 0)
        {
            *ptr++ = pad;
        }
    }

    // 如果数字个数小于精度，则填入(精度值 - i)个'0'
    while (i < precision--)
    {
        *ptr++ = '0';
    }

    // 输出数字
    while (i-- > 0)
    {
        *ptr++ = tmp[i];
    }

    // 如果宽度大于0，是左对齐，则填充空格到末尾
    while (size-- > 0)
    {
        *ptr++ = ' ';
    }

    return ptr;
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
    int len; // 用于存储长度
    int i;   // 循环变量

    char *str; // 用于存储buf指针
    char *s;   // 用于存储字符串
    int *ip;   // 用于存储n转化符的结果

    int flags; // number函数使用的标志

    int field_width; // 输出字段宽度
    int precision;   // 最小数字输出长度或最大字符串个数
    int qualifier;   // h, l或L 用于整数字段
    u32 num;
    u8 *ptr;

    // 首先将字符指针指向 buf
    // 然后扫描格式字符串，
    // 对各个格式转换指示进行相应的处理
    for (str = buf; *fmt; fmt++)
    {
        // 如果不是%开头，则不是转化串，直接输出到buf
        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }

        // 接下来取标志
        flags = 0;

    repeat:
        // 过滤掉前面的%
        ++fmt;
        switch (*fmt)
        {
            // 左对齐
        case '-':
            flags |= LEFT;
            goto repeat;
            // 符号位输出
        case '+':
            flags |= PLUS;
            goto repeat;
            // 符号位空格填充
        case ' ':
            flags |= SPACE;
            goto repeat;
            // 特殊进制转化
        case '#':
            flags |= SPECIAL;
            goto repeat;
            // 0填充，否则是空格
        case '0':
            flags |= ZERO_PAD;
            goto repeat;
        }

        // 接下来取宽度
        field_width = -1;

        // 如果是数字，则是显示宽度
        if (is_digit(*fmt))
        {
            field_width = skip_atoi(&fmt);
        }
        // 如果是*，这从参数中取出宽度
        else if (*fmt == '*')
        {
            ++fmt;
            field_width = va_arg(args, int);

            // 如果宽度是负数，则是左对齐和正数的宽度
            if (field_width < 0)
            {
                flags |= LEFT;
                field_width = -field_width;
            }
        }

        // 接下来取精度
        precision = -1;

        if (*fmt == '.')
        {
            ++fmt;
            // 如果是数字，则取出数字作为精度
            if (is_digit(*fmt))
            {
                precision = skip_atoi(&fmt);
            }
            // 如果是*，这从参数中获取精度
            else if (*fmt == '*')
            {
                precision = va_arg(args, int);
                ++fmt;
            }
            // 如果精度是负数，这忽略精度，当作0
            if (precision < 0)
            {
                precision = 0;
            }
        }

        // todo 目前暂时不用 下面这段代码分析长度修饰符，并将其存入 qualifer 变量
        qualifier = -1;

        // 接下来取长度修饰指示符
        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
        {
            qualifier = *fmt;
            ++fmt;
        }

        // 下面分析转化指示符
        switch (*fmt)
        {
            // 参数将被转换成**无符号**字符并输出转换结果
        case 'c':
            // 如果不是左对齐
            if (!(flags & LEFT))
            {
                // 这输出（宽度-1）个空格在前面
                while (--field_width > 0)
                {
                    *str++ = ' ';
                }
            }
            // 放入参数字符
            *str++ = (unsigned char)va_arg(args, int);
            // 如果宽度还大于0，则是右对齐，后面填充空格
            while (--field_width > 0)
            {
                *str++ = ' ';
            }
            break;
            // 如果转换指示符是 's'，则表示对应参数是字符串
        case 's':
            s = va_arg(args, char *);
            len = strlen(s);
            // 如果没有精度域，取字符串长度
            if (precision < 0)
            {
                precision = len;
            }
            // 如果字符串长度超过了精度域，则只输出精度长度
            else if (len > precision)
            {
                len = precision;
            }

            // 如果不是左对齐
            if (!(flags & LEFT))
            {
                // 如果长度小于显示宽度，则填充前面(宽度值-字符串长度) 个空格字符
                while (len < field_width--)
                {
                    *str++ = ' ';
                }
            }
            for (i = 0; i < len; i++)
            {
                *str++ = *s++;
            }

            // 如果长度还小于显示宽度，则是右对齐，填充(宽度值-字符串长度) 个空格字符到后面
            while (len < field_width--)
            {
                *str++ = ' ';
            }
            break;
            // 将无符号的整数转换为无符号八进制
        case 'o':
            num = va_arg(args, unsigned long);
            str = number(str, &num, 8, field_width, precision, flags);
            break;
            // 如果格式转换符是'p'，表示对应参数的一个指针类型
        case 'p':
            // 如果精度未设置过，则需要0填充，默认8位
            if (precision == -1)
            {
                precision = 8;
                flags |= ZERO_PAD;
            }
            num = va_arg(args, unsigned long);
            str = number(str, &num, 16, field_width, precision, flags);
            break;
            // 若格式转换指示是 'x' 或 'X'
            // 则表示对应参数需要打印成十六进制数输出
        case 'x':
            // 小写16进制输出
            flags |= SMALL;
        case 'X':
            num = va_arg(args, unsigned long);
            str = number(str, &num, 16, field_width, precision, flags);
            break;
            // 如果格式转换字符是'd', 'i' 或 'u'，则表示对应参数是整数
        case 'd':
        case 'i':
            // 有符号10进制输出
            flags |= SIGN;
            // 无符号10进制输出
        case 'u':
            num = va_arg(args, unsigned long);
            str = number(str, &num, 10, field_width, precision, flags);
            break;
            // 表示要把到目前为止转换输出的字符数保存到对应参数指针指定的位置中
        case 'n':
            ip = va_arg(args, int *);
            *ip = (str - buf);
            break;
        case 'f':
            flags |= SIGN;
            flags |= DOUBLE;
            double dnum = va_arg(args, double);
            str = number(str, (u32 *)&dnum, 10, field_width, precision, flags);
            break;
        case 'b': // binary
            num = va_arg(args, unsigned long);
            str = number(str, &num, 2, field_width, precision, flags);
            break;
        case 'm': // mac address
            flags |= SMALL | ZERO_PAD;
            ptr = va_arg(args, char *);
            for (size_t t = 0; t < 6; t++, ptr++)
            {
                int num = *ptr;
                str = number(str, &num, 16, 2, precision, flags);
                *str = ':';
                str++;
            }
            str--;
            break;
        case 'r': // ip address
            flags |= SMALL;
            ptr = va_arg(args, u8 *);
            for (size_t t = 0; t < 4; t++, ptr++)
            {
                int num = *ptr;
                str = number(str, &num, 10, field_width, precision, flags);
                *str = '.';
                str++;
            }
            str--;
            break;
        default:
            // 若格式转换符不是 '%'，则表示格式字符串有错
            if (*fmt != '%')
            {
                // 直接将%输出
                *str++ = '%';
            }
            // 如果格式转换符的位置处还有字符，则也直接将该字符写入输出串中
            // 然后继续循环处理格式字符串
            if (*fmt)
            {
                *str++ = *fmt;
            }
            // 否则到了末尾，退出循环
            else
            {
                --fmt;
            }
            break;
        }
    }
    // 最后在转化好的字符串结尾处添加上字符串结束标志
    *str = '\0';

    // 返回转化好的字符串长度值
    i = str - buf;

    assert(i < 1024);

    return i;
}
int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vsprintf(buf, fmt, args);
    va_end(args);
    return len;
}