#include <fs/std.h>
#include <mem/heap.h>
#include <gui/screen.h>

ssize_t stdoutWrite(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    UNUSED(node);
    UNUSED(offset);
        
    char *buf = (char *)buffer;
    for (size_t i = 0; i < size; i++)
        screen_putc(buf[i]);
    
    return size;
}

ssize_t stdinRead(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    UNUSED(node);
    UNUSED(offset);
    UNUSED(size);
    UNUSED(buffer);

    return 0;
}

VfsNode_t *createStdinNode()
{
    VfsNode_t *node = (VfsNode_t *)kcalloc(sizeof(VfsNode_t));
    if (!node)
        return NULL;
    
    node->flags = FS_FILE;
    node->read = stdinRead;
    return node;
}

VfsNode_t *createStdoutNode()
{
    VfsNode_t *node = (VfsNode_t *)kcalloc(sizeof(VfsNode_t));
    if (!node)
        return NULL;

    node->flags = FS_FILE;
    node->write = stdoutWrite;
    return node;
}

VfsNode_t *createStderrNode()
{
    return createStdoutNode();
}