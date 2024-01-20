#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <mem/heap.h>
#include <assert.h>
#include <logger.h>

int sys_open(const char *path, int flags, int mode)
{
    LOG_PROC("sys_open path `%s` with flags %d (mode %d)\n", path, flags, mode);
    
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
    
    node->attr = flags;
    vfs_open(node, flags);
    
    return process_add_file(_CurrentProcess, node);
}