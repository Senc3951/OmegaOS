#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <logger.h>

ssize_t sys_read(uint32_t fd, void *buf, size_t count)
{
    LOG_PROC("sys_read from file %u to %p (%llu bytes)\n", fd, buf, count);
    if (!buf)
        return -EINVAL;
    if (fd >= _CurrentProcess->fdt->length)
        return -ENOENT;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_read(node, node->offset, count, buf);
}