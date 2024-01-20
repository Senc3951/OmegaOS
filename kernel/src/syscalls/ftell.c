#include <fs/vfs.h>
#include <logger.h>

long sys_ftell(uint32_t fd)
{
    LOG_PROC("sys_ftell from file %u\n", fd);
    if (fd >= _CurrentProcess->fdt->length)
        return -ENOENT;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_ftell(node);
}