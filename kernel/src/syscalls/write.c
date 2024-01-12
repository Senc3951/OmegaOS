#include <sys/syscalls.h>
#include <fs/vfs.h>
#include <gui/screen.h>
#include <logger.h>

ssize_t sys_write(uint32_t fd, const void *buf, size_t count)
{
    LOG_PROC("sys_write to file %u from %p (%llu bytes)\n", fd, buf, count);
    switch (fd)
    {
        case STDOUT:
            for (size_t i = 0; i < count; i++)
                screen_putc(((const char *)buf)[i]);
            
            return count;
        case STDIN:
        case STDERR:
            return 0;
    }
    
    if (fd >= _CurrentProcess->fdt->size)
        return -ENOENT;;

    VfsNode_t *node = _CurrentProcess->fdt->nodes[fd];
    return vfs_write(node, node->offset, count, (void *)buf);
}