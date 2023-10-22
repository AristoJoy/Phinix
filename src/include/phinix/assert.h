#ifndef PHINIX_ASSERT_H
#define PHINIX_ASSERT_H

/**
 * @brief
 *
 * @param exp 表达式字符串
 * @param file 文件名P
 * @param base 文件名
 * @param line 行数
 */
void assertion_failure(char *exp, char *file, char *base, int line);

#define assert(exp) \
    if (exp)        \
        ;           \
    else            \
        assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__)

void panic(const char *fmt, ...);

#endif