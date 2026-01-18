#include <lib.h>
#include <print.h>


void putchar(char c) {
    write(1, &c, 1); // 文件描述符 1 表示标准输出
}