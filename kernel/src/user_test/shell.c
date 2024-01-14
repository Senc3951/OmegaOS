#include <user_test/syscall.h>

void Xshell()
{
    int y = 6;
    char x[12] = "hello world";
    syscall3(1, 1, (uint64_t)x, sizeof(x));
    syscall1(4, y);
}