#include <fs/vfs.h>
#include <mem/heap.h>
#include <sys/scheduler.h>
#include <misc/list.h>
#include <libc/string.h>

VfsNode_t *_RootFS = NULL;

char *normalizePath(char *cwd, const char *path)
{
    char *output = NULL;
    list_t *lst = list_create();
    if (!lst)
        return NULL;
    
    if (*path && *path != FS_PATH_SEPERATOR)
    {
        // Path is relative, push working directory
        size_t len = strlen(cwd);
        char *cwdCopy = (char *)kmalloc(len + 1);
        if (!cwdCopy)
            goto cleanup;
        
        memcpy(cwdCopy, cwd, len + 1);
        char *token = strtok(cwdCopy, FS_PATH_SEPERATOR_STR);
        while (token)
        {
            list_insert(lst, strdup(token));
            token = strtok(NULL, FS_PATH_SEPERATOR_STR);
        }
        kfree(cwdCopy);
    }
    
    // Push the path
    size_t rlpLen = strlen(path);
    char *rlpCopy = (char *)kmalloc(rlpLen + 1);
    if (!rlpCopy)
        goto cleanup;
    
    memcpy(rlpCopy, path, rlpLen + 1);
    char *token = strtok(rlpCopy, FS_PATH_SEPERATOR_STR);
    while (token)
    {
        if (!strcmp(token, FS_PATH_UP_DIR))
        {
            // Remove the last directory pushed
            node_t *n = list_pop(lst);
            if (n)
            {
                kfree(n->value);
                kfree(n);
            }
        }
        else if (!strcmp(token, FS_PATH_CURR_DIR)) { }
        else
            list_insert(lst, strdup(token));
        
        token = strtok(NULL, FS_PATH_SEPERATOR_STR);
    }
    kfree(rlpCopy);

    // Assembly the final path
    size_t fSize = 0;
    foreach(item, lst) {
        fSize += strlen(item->value) + 1;
    }
    
    output = (char *)kmalloc(fSize + 1);
    if (!output)
        goto cleanup;
    
    if (fSize == 0)
    {
        // Root directory
        output = (char *)krealloc(output, 2);
        output[0] = FS_PATH_SEPERATOR;
        output[1] = '\0';
    }
    else
    {
        char *off = output;
        foreach(item, lst) {
            *off++ = FS_PATH_SEPERATOR;
            memcpy(off, item->value, strlen(item->value) + 1);
            
            off += strlen(item->value);
        }
    }
    
cleanup:
    list_destroy(lst);
    list_free(lst);
    kfree(lst);
    return output;
}

VfsNode_t *vfs_openFile(const char *name, uint32_t attr)
{
    char *cwd = currentProcess()->cwd;
    char *path = normalizePath(cwd, name);
    if (!path)
        return NULL;

    size_t plen = strlen(path);
    VfsNode_t *newNode = (VfsNode_t *)kmalloc(sizeof(VfsNode_t));
    if (!newNode)
    {
        kfree(path);
        return NULL;
    }
    
    if (plen == 1)  // Root path
    {
        memcpy(newNode, _RootFS, sizeof(VfsNode_t));
        return newNode;
    }

    char *pathOffset = path;
    uint32_t pathDepth = 0;
    while (pathOffset < path + plen)
    {
        if (*pathOffset == FS_PATH_SEPERATOR)
        {
            *pathOffset = '\0';
            pathDepth++;
        }
        
        pathOffset++;
    }
    path[plen] = '\0';
    pathOffset = path + 1;

    VfsNode_t *mount = _RootFS;
    memcpy(newNode, mount, sizeof(VfsNode_t));
    for (uint32_t depth = 0; depth < pathDepth; depth++)
    {
        VfsNode_t *tmpNode = vfs_finddir(newNode, pathOffset);
        kfree(newNode);
        newNode = tmpNode;

        if (!newNode)
        {
            kfree(path);
            return NULL;
        }
        else if (depth == pathDepth - 1)
        {
            vfs_open(newNode, attr);
            kfree(path);
            return newNode;
        }
        
        pathOffset += strlen(pathOffset) + 1;
    }
    
    kfree(path);
    return newNode;
}

ssize_t vfs_read(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    if (!node || !node->read || (node->attr & O_WRONLY))
        return -EPERM;
    if ((node->flags & FS_FILE) != FS_FILE)
        return -EISDIR;
    if (!size)
        return 0;
    
    return node->read(node, offset, size, buffer);
}

ssize_t vfs_write(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    if (!node || !node->write || (node->attr & O_RDONLY))
        return -EPERM;
    if ((node->flags & FS_FILE) != FS_FILE)
        return -EISDIR;
    if (!size)
        return 0;
    
    return node->write(node, offset, size, buffer);
}

void vfs_open(VfsNode_t *node, uint32_t attr)
{
    if (node && node->open)
        node->open(node, attr);
}

void vfs_close(VfsNode_t *node)
{
    if (node && node->close && node != _RootFS)
        node->close(node);
}

struct dirent *vfs_readdir(VfsNode_t *node, uint32_t index)
{
    if (!node || !node->readdir)
        return NULL;
    if ((node->flags & FS_DIR) != FS_DIR)
        return NULL;
    
    return node->readdir(node, index);
}

VfsNode_t *vfs_finddir(VfsNode_t *node, const char *name)
{
    if (!node || !node->finddir)
        return NULL;
    if ((node->flags & FS_DIR) != FS_DIR)
        return NULL;
    
    return node->finddir(node, name);
}

static int getParent(const char *name, uint32_t attr, VfsNode_t **parent, const char **fileName)
{
    char *cwd = currentProcess()->cwd;
    char *path = normalizePath(cwd, name);    
    if (!path)
        return EPERM;

    for (long i = strlen(path) - 1; i >= 0; i--)
    {
        if (path[i] == FS_PATH_SEPERATOR)
        {
            *fileName = name + i;
            break;
        }
    }
    
    size_t plen = strlen(path);
    char *parentPath = (char *)kmalloc(plen + 4);
    if (!parentPath)
        return ENOMEM;
    
    strcpy(parentPath, path);
    strcpy(parentPath + plen, "/..");

    char *tmp = path + plen - 1;
    while (tmp > path)
    {
        if (*tmp == FS_PATH_SEPERATOR)
        {
            tmp++;
            break;
        }

        tmp--;
    }
    while (*tmp == FS_PATH_SEPERATOR)
        tmp++;
    
    *parent = vfs_openFile(parentPath, attr);
    kfree(parentPath);
    kfree(path);
        
    return *parent ? ENOER : EPERM;
}

int vfs_create(const char *name, uint32_t attr)
{
    VfsNode_t *parent = NULL;
    const char *fileName = NULL;
    int ret = getParent(name, attr, &parent, &fileName);
    
    if (ret != ENOER)
        return ret;
    if (!parent || !fileName)
        return EPERM;
    
    if (parent->create)
        ret = parent->create(parent, fileName, attr);
    else
        ret = EPERM;
    
    kfree(parent);
    return ret;
}

int vfs_mkdir(const char *name, uint32_t attr)
{
    VfsNode_t *parent = NULL;
    const char *fileName = NULL;
    int ret = getParent(name, attr, &parent, &fileName);

    if (ret != ENOER)
        return ret;
    if (!parent || !fileName)
        return EPERM;
    
    if (parent->mkdir)
        ret = parent->mkdir(parent, fileName, attr);
    else
        ret = EPERM;
    
    kfree(parent);
    return ret;
}

long vfs_ftell(VfsNode_t *node)
{
    if (!node)
        return -EPERM;
    if ((node->flags & FS_FILE) != FS_FILE)
        return -EISDIR;
    
    return node->offset;
}

int vfs_fseek(VfsNode_t *node, long offset, int whence)
{
    if (!node)
        return EPERM;
    if ((node->flags & FS_FILE) != FS_FILE)
        return EISDIR;
    
    switch (whence)
    {
        case SEEK_SET:
            node->offset = offset;
            break;
        case SEEK_CUR:
            node->offset += offset;
            break;
        case SEEK_END:
            node->offset = node->size;
            break;
        default:
            return EINVAL;
    }
    
    return ENOER;
}

int vfs_delete(const char *name)
{
    VfsNode_t *parent = NULL;
    const char *fileName = NULL;
    int ret = getParent(name, 0, &parent, &fileName);

    if (ret != ENOER)
        return ret;
    if (!parent || !fileName)
        return EPERM;
    
    if (parent->delete)
        ret = parent->delete(parent, fileName);
    else
        ret = EPERM;
    
    kfree(parent);
    return ret;
}