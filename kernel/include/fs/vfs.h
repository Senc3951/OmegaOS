#pragma once

#include <common.h>

#define FS_MAX_PATH             256
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

#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

#define S_IXOTH   0x1
#define S_IWOTH   0x2
#define S_IROTH   0x4
#define S_IRWXO   (S_IXOTH | S_IWOTH | S_IROTH)
#define S_IXGRP   0x10
#define S_IWGRP   0x20
#define S_IRGRP   0x40
#define S_IRWXG   (S_IXGRP | S_IWGRP | S_IRGRP)
#define S_IXUSR   0x100
#define S_IWUSR   0x200
#define S_IRUSR   0x400
#define S_IRWXU   (S_IXUSR | S_IWUSR | S_IRUSR)

#define	FREAD		0x0001
#define	FWRITE		0x0002
#define	O_NONBLOCK	0x0004		/* no delay */
#define	O_APPEND	0x0008		/* set append mode */
#define	O_SHLOCK	0x0010		/* open with shared file lock */
#define	O_EXLOCK	0x0020		/* open with exclusive file lock */
#define	O_ASYNC		0x0040		/* signal pgrp when data ready */
#define	O_FSYNC		0x0080		/* synchronous writes */
#define	O_SYNC		0x0080		/* POSIX synonym for O_FSYNC */
#define	O_NOFOLLOW	0x0100		/* don't follow symlinks */
#define	O_CREAT		0x0200		/* create if nonexistent */
#define	O_TRUNC		0x0400		/* truncate to zero length */
#define	O_EXCL		0x0800		/* error if already exists */

typedef struct VFS_NODE VfsNode_t;

typedef ssize_t (*read_type_t)(VfsNode_t *,  uint32_t, size_t, void *);
typedef ssize_t (*write_type_t)(VfsNode_t *, uint32_t, size_t, void *);
typedef void (*open_type_t)(VfsNode_t *, uint32_t);
typedef void (*close_type_t)(VfsNode_t *);
typedef struct dirent *(*readdir_type_t)(VfsNode_t *, uint32_t);
typedef VfsNode_t *(*finddir_type_t)(VfsNode_t *, const char *);
typedef int (*create_type_t)(VfsNode_t *, const char *, uint32_t);
typedef int (*mkdir_type_t)(VfsNode_t *, const char *, uint32_t);
typedef int (*delete_type_t)(VfsNode_t *, const char *);

struct VFS_NODE
{
    char name[FS_MAX_PATH];
    uint32_t attr;  /* Attributes of file. */
    uint32_t uid;
    uint32_t gid;
    uint32_t flags; /* Type of file. */
    uint32_t inode;
    uint32_t size;
    long offset;
    
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
    delete_type_t delete;
};

typedef struct dirent
{
    uint32_t ino;
    char name[FS_MAX_PATH];
} dirent;

extern VfsNode_t *_RootFS;

/// @brief Get a VfsNode of a file.
/// @param name Name of the file.
/// @param attr Attributes to open the file with.
/// @return VfsNode of the file, NULL, if failed.
VfsNode_t *vfs_openFile(const char *name, uint32_t attr);

/// @brief Read from a node.
/// @param node Node to read from.
/// @param offset Offset to read from.
/// @param size Amount of bytes to read.
/// @param buffer Buffer to read to.
/// @return Bytes read.
ssize_t vfs_read(VfsNode_t *node, uint32_t offset, size_t size, void *buffer);

/// @brief Write to a node.
/// @param node Node to write to.
/// @param offset Offset to write to.
/// @param size Amount of bytes to write.
/// @param buffer Buffer to write.
/// @return Bytes written.
ssize_t vfs_write(VfsNode_t *node, uint32_t offset, size_t size, void *buffer);

/// @brief Open a file.
/// @param node Node to open.
/// @param attr Attributes to open the file with.
void vfs_open(VfsNode_t *node, uint32_t attr);

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

/// @brief Get the size of a file.
/// @param node File to get the size of.
/// @return Size of the file.
long vfs_ftell(VfsNode_t *node);

/// @brief Set the offset of a file.
/// @param node File to set the offset of.
/// @param offset Relative offset to set.
/// @param whence From where to count the relative offset.
int vfs_fseek(VfsNode_t *node, long offset, int whence);

/// @brief Delete a file.
/// @param name Name of the file to delete.
int vfs_delete(const char *name);