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
    
    vfs_open(node, flags);
    return process_add_file(_CurrentProcess, node);
}