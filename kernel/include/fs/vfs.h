#pragma once

#include <common.h>

#define FS_MAX_NAME 256

#define FS_PATH_SEPERATOR       '/'
#define FS_PATH_SEPERATOR_STR   "/"
#define FS_PATH_CURR_DIR        "."
#define FS_PATH_UP_DIR          ".."

#define FS_FILE     0x1
#define FS_DIR      0x2
#define FS_CHARDEV  0x4
#define FS_BLKDEV   0x8
#define FS_FIFO     0x10
#define FS_SOCK     0x20
#define FS_SYMLINK  0x40

typedef struct VFS_NODE VfsNode_t;

typedef uint32_t (*read_type_t) (VfsNode_t *,  uint32_t, size_t, void *);
typedef uint32_t (*write_type_t) (VfsNode_t *, uint32_t, size_t, void *);
typedef void (*open_type_t) (VfsNode_t *, uint8_t read, uint8_t write);
typedef void (*close_type_t) (VfsNode_t *);
typedef struct dirent *(*readdir_type_t) (VfsNode_t *, uint32_t);
typedef VfsNode_t *(*finddir_type_t) (VfsNode_t *, const char *name);
typedef int (*create_type_t) (VfsNode_t *, const char *name, uint32_t permission);
typedef int (*mkdir_type_t) (VfsNode_t *, const char *name, uint32_t permission);

struct VFS_NODE
{
    char name[FS_MAX_NAME];
    uint32_t attr;
    uint32_t uid;
    uint32_t gid;
    uint32_t flags;
    uint32_t inode;
    uint32_t length;
    uint32_t openFlags;

    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;

    read_type_t read;
    write_type_t write;
    open_type_t open;
    close_type_t close;
    readdir_type_t readdir;
    finddir_type_t finddir;
    create_type_t create;
    mkdir_type_t mkdir;
};

typedef struct dirent
{
    uint32_t ino;
    char name[FS_MAX_NAME];
} dirent;

extern VfsNode_t *_RootFS;

/// @brief Read from a node.
/// @param node Node to read from.
/// @param offset Offset to read from.
/// @param size Amount of bytes to read.
/// @param buffer Buffer to read to.
/// @return Bytes read.
uint32_t vfs_read(VfsNode_t *node, uint32_t offset, size_t size, void *buffer);

/// @brief Write to a node.
/// @param node Node to write to.
/// @param offset Offset to write to.
/// @param size Amount of bytes to write.
/// @param buffer Buffer to write.
/// @return Bytes written.
uint32_t vfs_write(VfsNode_t *node, uint32_t offset, size_t size, void *buffer);

/// @brief Open a file.
/// @param node Node to open.
/// @param read Read access.
/// @param write Write access.
void vfs_open(VfsNode_t *node, uint8_t read, uint8_t write);

/// @brief Close a file.
/// @param node Node to close.
void vfs_close(VfsNode_t *node);

/// @brief Read from a directory.
/// @param node Directory to read from.
/// @param index Index in the directory to read.
struct dirent *vfs_readdir(VfsNode_t *node, uint32_t index);

/// @brief Find an entry in a directory.
/// @param node Directory to search in.
/// @param name Name of the file to search for.
/// @return Found file, NULL, otherwise.
VfsNode_t *vfs_finddir(VfsNode_t *node, const char *name);

/// @brief Create a file.
/// @param name Name of the file.
/// @param attr Attributes of the file.
/// @return Status of the operation.
int vfs_create(const char *name, uint32_t attr);

/// @brief Create a directory.
/// @param name Name of the directory.
/// @param attr Attributes of the directory.
/// @return Status of the operation.
int vfs_mkdir(const char *name, uint32_t attr);