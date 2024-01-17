#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <mem/heap.h>
#include <logger.h>

DIR *sys_opendir(const char *name)
{
    LOG_PROC("sys_opendir directory `%s`\n", name);
    
    // Check that the directory exists
    VfsNode_t *node = vfs_openFile(name, 0);
    if (!node)
        return NULL;
    
    int64_t fd = process_add_file(_CurrentProcess, node);
    if (fd < 0)
    {
        kfree(node);
        return NULL;
    }

    DIR *dir = (DIR *)kmalloc(sizeof(DIR));
    if (!dir)
    {
        kfree(node);
        return NULL;
    }
    
    dir->i = 0;
    dir->fd = (uint32_t)fd;
    return dir;
}