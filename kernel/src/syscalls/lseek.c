#include <fs/vfs.h>
#include <logger.h>

int sys_lseek(uint32_t fd, long offset, int whence)
{
    LOG_PROC("sys_lseek from file %u with offset %ld (whence %d)\n", fd, offset, whence);
    if (fd >= _CurrentProcess->fdt->size)
        return -ENOENT;
    
    VfsNode_t *node = _CurrentProcess->fdt->nodes[fd];
    return vfs_fseek(node, offset, whence);
}