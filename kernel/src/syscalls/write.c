#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <logger.h>

ssize_t sys_write(uint32_t fd, const void *buf, size_t count)
{
    if (fd > 2)
        LOG_PROC("sys_write to file %u from %p (%llu bytes)\n", fd, buf, count);
    if (!buf)
        return -EINVAL;
    if (fd >= _CurrentProcess->fdt->length)
        return -ENOENT;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_write(node, node->offset, count, (void *)buf);
}