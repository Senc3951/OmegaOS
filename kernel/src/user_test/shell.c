#include <user_test/unistd.h>

void shell()
{
    int y;
    char x[12] = "hello world";
    
    y = write(STDOUT, x, sizeof(x));
    exit(y);
}