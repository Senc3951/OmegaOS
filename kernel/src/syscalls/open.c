#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <mem/heap.h>
#include <assert.h>
#include <logger.h>

uint32_t sys_open(const char *path, int flags, int mode)
{
    UNUSED(mode);        
    LOG_PROC("sys_open path `%s` with flags %d\n", path, flags);
    
    VfsNode_t *node = vfs_openFile(path, flags);
    if (!node)
    {
        if ((flags & O_CREAT) != O_CREAT)
            return -ENOENT;

        int ret = vfs_create(path, flags);
        if (ret != ENOER)
            return -ret;

        node = vfs_openFile(path, flags);
        if (!node)
            return -EPERM;
        
    }
    
    if (_CurrentProcess->fdt->size == _CurrentProcess->fdt->capacity)
    {
        _CurrentProcess->fdt->capacity *= 2;
        _CurrentProcess->fdt->nodes = (VfsNode_t **)krealloc(_CurrentProcess->fdt->nodes, sizeof(VfsNode_t *) * _CurrentProcess->fdt->capacity);
        if (!_CurrentProcess->fdt->nodes)
            return -ENOMEM;
    }
        
    _CurrentProcess->fdt->nodes[_CurrentProcess->fdt->size++] = node;
    return _CurrentProcess->fdt->size - 1;
}