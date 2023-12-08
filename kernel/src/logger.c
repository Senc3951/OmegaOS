#include <logger.h>
#include <io/serial.h>

void __kfctprintf(char c, void *arg)
{
    UNUSED(arg);
    
#ifdef DEBUG
    serial_write(c);
#endif
}