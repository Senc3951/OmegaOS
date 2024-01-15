#include <fs/vfs.h>
#include <logger.h>

int sys_mkdir(const char *name, uint32_t attr)
{
    LOG_PROC("sys_mkdir directory `%s` with attributes %u\n", name, attr);
    return vfs_mkdir(name, attr);
}