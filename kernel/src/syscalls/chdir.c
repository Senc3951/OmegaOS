#include <fs/vfs.h>
#include <mem/heap.h>
#include <logger.h>
#include <libc/string.h>

int sys_chdir(const char *path)
{
    LOG_PROC("sys_chdir to path %s\n", path);
    if (!path)
        return EINVAL;

    char *newPath = normalizePath(_CurrentProcess->cwd, path);
    if (!newPath)
        return ENOMEM;
    
    // Attempt to open the path
    VfsNode_t *node = vfs_openFile(newPath, 0);
    if (!node)
    {
        kfree(newPath);
        return ENOENT;
    }
    if (!(node->flags & FS_DIR))
    {
        kfree(node);
        kfree(newPath);

        return ENOTDIR;
    }
    kfree(node);
    
    memset(_CurrentProcess->cwd, 0, FS_MAX_PATH);
    strncpy(_CurrentProcess->cwd, newPath, FS_MAX_PATH);
    kfree(newPath);
    
    return ENOER;
}