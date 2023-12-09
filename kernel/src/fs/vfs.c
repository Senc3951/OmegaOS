#include <fs/vfs.h>
#include <mem/heap.h>
#include <list.h>
#include <libc/string.h>

VfsNode_t *_RootFS = NULL;

static char *fixPath(char *cwd, const char *path)
{
    list_t *lst = list_create();
    if (*path && *path != FS_PATH_SEPERATOR)
    {
        // Path is relative, push working directory
        size_t len = strlen(cwd);
        char *cwdCopy = (char *)kmalloc(len + 1);
        if (!cwdCopy)
            return NULL;
        
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
        return NULL;
    
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
    
    char *output = (char *)kmalloc(fSize + 1);
    if (!output)
        return NULL;
    
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
    
    list_destroy(lst);
    list_free(lst);
    kfree(lst);
    return output;
}

static VfsNode_t *intrnOpen(const char *filename, uint32_t flags)
{
    char *cwd = "/";
    char *path = fixPath(cwd, filename);
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
            vfs_open(newNode, 1, 0);
            kfree(path);

            return newNode;
        }
        
        pathOffset += strlen(pathOffset) + 1;
    }
    
    kfree(path);
    return newNode;
}

uint32_t vfs_read(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    if (!node || !node->read)
        return 0;
    
    return node->read(node, offset, size, buffer);
}

uint32_t vfs_write(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    if (!node || !node->write)
        return 0;
    
    return node->write(node, offset, size, buffer);
}

void vfs_open(VfsNode_t *node, uint8_t read, uint8_t write)
{
    if (node && node->open)
        node->open(node, read, write);
}

void vfs_close(VfsNode_t *node)
{
    if (node && node->close && node != _RootFS)
        node->close(node);
}

struct dirent *vfs_readdir(VfsNode_t *node, uint32_t index)
{
    if (!node)
        return NULL;
    
    if ((node->flags & FS_DIR) && node->readdir)
        return node->readdir(node, index);
    
    return NULL;
}

VfsNode_t *vfs_finddir(VfsNode_t *node, const char *name)
{
    if (!node)
        return NULL;
    
    if ((node->flags & FS_DIR) && node->finddir)
        return node->finddir(node, name);
    
    return NULL;
}

int vfs_create(const char *name, uint32_t attr)
{
    char *cwd = "/";    // For now
    char *path = fixPath(cwd, name);    
    if (!path)
        return -1;
    
    size_t plen = strlen(path);
    char *parentPath = (char *)kmalloc(plen + 4);
    if (!parentPath)
        return -2;

    memcpy(parentPath, path, plen);
    strcat(parentPath, "/..");
    parentPath[plen + 3] = '\0';

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

    VfsNode_t *parent = intrnOpen(parentPath, 0);
    kfree(parentPath);

    if (!parent)
    {
        kfree(path);
        return -3;
    }

    int ret = -4;
    if (parent->create)
        ret = parent->create(parent, tmp, attr);
    
    kfree(parent);
    kfree(path);
    
    return ret;
}

int vfs_mkdir(const char *name, uint32_t attr)
{
    char *cwd = "/";    // For now
    char *path = fixPath(cwd, name);    
    if (!path)
        return -1;

    size_t plen = strlen(path);
    char *parentPath = (char *)kmalloc(plen + 4);
    if (!parentPath)
        return -2;
    
    memcpy(parentPath, path, plen);
    strcat(parentPath, "/..");
    parentPath[plen + 3] = '\0';

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

    // Check for existance
    VfsNode_t *thisNode = intrnOpen(path, 0);
    if (thisNode)
    {
        vfs_close(thisNode);
        kfree(path);
        
        return -3;
    }

    VfsNode_t *parent = intrnOpen(parentPath, 0);
    kfree(parentPath);
    if (!parent)
    {
        kfree(path);
        return -4;
    }

    int ret = -5;
    if (parent->mkdir)
        ret = parent->mkdir(parent, tmp, attr);
    
    kfree(parent);
    kfree(path);
    
    return ret;
}