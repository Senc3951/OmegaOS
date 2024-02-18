#include <sys/scheduler.h>
#include <fs/vfs.h>
#include <mem/heap.h>
#include <assert.h>
#include <logger.h>
#include <libc/string.h>

int sys_open(const char *path, int flags, int mode)
{
    LOG_PROC("sys_open path `%s` with flags %d (mode %d)\n", path, flags, mode);
    if (!path)
        return -EINVAL;
    
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
    
    return process_add_file(currentProcess(), node);
}

int sys_close(uint32_t fd)
{
    LOG_PROC("sys_close file %u\n", fd);
    return process_close_file(currentProcess(), fd) ? ENOER : -ENOENT;
}

ssize_t sys_read(uint32_t fd, void *buf, size_t count)
{
    if (fd > 2)
        LOG_PROC("sys_read from file %u to %p (%llu bytes)\n", fd, buf, count);
    if (!buf)
        return -EINVAL;
    if (fd >= currentProcess()->fdt->length)
        return -ENOENT;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_read(node, node->offset, count, buf);
}

ssize_t sys_write(uint32_t fd, const void *buf, size_t count)
{
    if (fd > 2)
        LOG_PROC("sys_write to file %u from %p (%llu bytes)\n", fd, buf, count);
    if (!buf)
        return -EINVAL;
    if (fd >= currentProcess()->fdt->length)
        return -ENOENT;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_write(node, node->offset, count, (void *)buf);
}

DIR *sys_opendir(const char *name)
{
    LOG_PROC("sys_opendir directory `%s`\n", name);
    if (!name)
        return NULL;
    
    // Check that the directory exists
    VfsNode_t *node = vfs_openFile(name, 0);
    if (!node)
        return NULL;
    
    int64_t fd = process_add_file(currentProcess(), node);
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
    
    dir->currentEntry = 0;
    dir->fd = (uint32_t)fd;
    return dir;
}

void sys_closedir(DIR *dirp)
{
    uint32_t fd = dirp->fd;
    LOG_PROC("sys_closedir directory %u\n", fd);
    if (!dirp)
        return;
    
    process_close_file(currentProcess(), fd);
    kfree(dirp);
}

struct dirent *sys_readdir(DIR *dirp)
{
    uint32_t fd = dirp->fd;
    LOG_PROC("sys_readdir directory %u (index %u)\n", fd, dirp->currentEntry);
    
    if (!dirp)
        return NULL;
    if (fd >= currentProcess()->fdt->length)
        return NULL;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_readdir(node, dirp->currentEntry++);
}

long sys_ftell(uint32_t fd)
{
    LOG_PROC("sys_ftell from file %u\n", fd);
    if (fd >= currentProcess()->fdt->length)
        return -ENOENT;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_ftell(node);
}

int sys_lseek(uint32_t fd, long offset, int whence)
{
    LOG_PROC("sys_lseek from file %u with offset %ld (whence %d)\n", fd, offset, whence);
    if (fd >= currentProcess()->fdt->length)
        return -ENOENT;
    
    VfsNode_t *node = PROC_FILE_AT(fd);
    return vfs_fseek(node, offset, whence);
}

int sys_chdir(const char *path)
{
    LOG_PROC("sys_chdir to path `%s`\n", path);
    if (!path)
        return EINVAL;
    
    Process_t *current = currentProcess();
    char *newPath = normalizePath(current->cwd, path);
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
    
    memset(current->cwd, 0, FS_MAX_PATH);
    strncpy(current->cwd, newPath, FS_MAX_PATH);
    kfree(newPath);
    
    return ENOER;
}

int sys_mkdir(const char *name, uint32_t attr)
{
    LOG_PROC("sys_mkdir directory `%s` with attributes %u\n", name, attr);
    if (!name)
        return EINVAL;
    
    return vfs_mkdir(name, attr);
}

char *sys_getcwd(char *buf, size_t size)
{
    LOG_PROC("sys_getcwd to %p with size %lu\n", buf, size);
    if (!buf || !size)
        return NULL;
    
    return strncpy(buf, currentProcess()->cwd, size);        
}