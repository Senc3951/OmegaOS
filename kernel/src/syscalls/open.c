#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <mem/heap.h>
#include <assert.h>
#include <logger.h>

int sys_open(const char *path, int flags, int mode)
{
    UNUSED(mode);
    LOG_PROC("sys_open file %s with flags %d\n", path, flags);
    
    int ret = vfs_create(path, flags);
    if (ret != ENOER)
        return -ret;
    
    VfsNode_t *node = vfs_openFile(path, flags);
    if (!node)
        return -ENOENT;
    
    if (_CurrentProcess->fdt->size == _CurrentProcess->fdt->capacity)
    {
        _CurrentProcess->fdt->capacity *= 2;
        _CurrentProcess->fdt->nodes = (VfsNode_t **)krealloc(_CurrentProcess->fdt->nodes, sizeof(VfsNode_t *) * _CurrentProcess->fdt->capacity);
        assert(_CurrentProcess->fdt->nodes);
    }
    
    _CurrentProcess->fdt->nodes[_CurrentProcess->fdt->size++] = node;
    return _CurrentProcess->fdt->size - 1;
}