#include <logger.h>
#include <io/serial.h>

void __kfctprintf(char c, void *arg)
{
    UNUSED(arg);
    serial_write(c);
}