#include <user_test/unistd.h>
#include <user_test/fcntl.h>

void shell()
{
    int r = mkdir("test",0);
    exit(r);
}