#include <sys/syscalls.h>
#include <fs/vfs.h>
#include <gui/screen.h>
#include <logger.h>

ssize_t sys_read(uint32_t fd, void *buf, size_t count)
{
    LOG_PROC("sys_read from file %u to %p (%llu bytes)\n", fd, buf, count);
    switch (fd)
    {
        case STDIN:
        case STDERR:
            LOG("TODO: sys_read STDIN\n");
            return 0;
        case STDOUT:
            return 0;
    }
    
    if (fd >= _CurrentProcess->fdt->size)
        return -ENOENT;;
    
    VfsNode_t *node = _CurrentProcess->fdt->nodes[fd];
    return vfs_read(node, node->offset, count, buf);
}