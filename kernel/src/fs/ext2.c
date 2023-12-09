#include <fs/ext2.h>
#include <drivers/storage/ide.h>
#include <mem/heap.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

#define SUPER_BLOCK_SECTOR  (1024 / ATA_SECTOR_SIZE)
#define BLOCK_GROUP_START   2
#define INODE_ROOT          2

static SuperBlock_t g_superBlock;
static BlockGroupDescriptor_t *g_blockGroupDescriptors;
static Inode_t *g_rootInode;
static uint32_t g_blockSize, g_blockGroupDescriptorCount;

#define SECTORS_PER_BLOCK       (g_blockSize / ATA_SECTOR_SIZE)
#define INODES_PER_BLOCK        (g_blockSize / g_superBlock.s_inode_size)
#define PTR_BLOCKS_PER_BLOCK    (g_blockSize / sizeof(uint32_t))
#define SINGLE_IND_PTR_BLOCKS   (PTR_BLOCKS_PER_BLOCK)
#define DOUBLE_IND_PTR_BLOCKS   (SINGLE_IND_PTR_BLOCKS * PTR_BLOCKS_PER_BLOCK)
#define TRIPLE_IND_PTR_BLOCKS   (DOUBLE_IND_PTR_BLOCKS * PTR_BLOCKS_PER_BLOCK)

#define DEF_TMP_BUF()                                       \
    static uint8_t *tmpBuf = NULL;                          \
    if (!tmpBuf)                                            \
        assert(tmpBuf = (uint8_t *)kmalloc(g_blockSize));   \

static uint32_t b2s(const uint32_t block)
{
    return block * SECTORS_PER_BLOCK;
}

static bool readBlock(const uint32_t block, void *buffer)
{
    return ide_read(b2s(block), buffer, SECTORS_PER_BLOCK);
}

static bool writeBlock(const uint32_t block, void *buffer)
{
    return ide_write(b2s(block), buffer, SECTORS_PER_BLOCK);
}

static Inode_t *readInode(const uint32_t ino)
{
    DEF_TMP_BUF();
    
    uint32_t group = (ino - 1) / g_superBlock.s_inodes_per_group;
    if (group >= g_blockGroupDescriptorCount)
        return NULL;
    
    uint32_t inodeTableStart = g_blockGroupDescriptors[group].bg_inode_table;
    uint32_t inodeIndex = (ino - 1) % g_superBlock.s_inodes_per_group;
    uint32_t block = inodeTableStart + inodeIndex / INODES_PER_BLOCK;
    if (!readBlock(block, tmpBuf))
        return NULL;
    
    Inode_t *inode = (Inode_t *)kmalloc(g_superBlock.s_inode_size);
    if (!inode)
        return NULL;
    
    uint32_t blockOffset = (inodeIndex % INODES_PER_BLOCK) * g_superBlock.s_inode_size;
    memcpy(inode, tmpBuf + blockOffset, g_superBlock.s_inode_size);
    
    return inode;
}

static bool readInodeBlock(Inode_t *inode, const uint32_t block, void *buffer)
{
    DEF_TMP_BUF();
    
    if (block < EXT2_NDIR_BLOCKS)   // Direct
        return readBlock(inode->i_block[block], buffer);
    else if (block < EXT2_IND_BLOCK + SINGLE_IND_PTR_BLOCKS)   // Single indirect
    {
        // Read single indirect block
        if (!readBlock(inode->i_block[EXT2_IND_BLOCK], tmpBuf))
            return false;
        
        // Read actual block
        uint32_t realBlock = ((uint32_t *)tmpBuf)[block - EXT2_IND_BLOCK];
        return readBlock(realBlock, buffer);
    }
    else if (block < EXT2_IND_BLOCK + DOUBLE_IND_PTR_BLOCKS + SINGLE_IND_PTR_BLOCKS)    // Double indirect
    {
        // Read double indirect block
        if (!readBlock(inode->i_block[EXT2_DIND_BLOCK], tmpBuf))
            return false;

        uint32_t b = (block - EXT2_IND_BLOCK) / PTR_BLOCKS_PER_BLOCK;
        uint32_t c = b / PTR_BLOCKS_PER_BLOCK;
        uint32_t d = b - c * PTR_BLOCKS_PER_BLOCK;

        // Read single indirect block
        uint32_t singleRealBlock = ((uint32_t *)tmpBuf)[c];
        if (!readBlock(singleRealBlock, tmpBuf))
            return false;
        
        uint32_t realBlock = ((uint32_t *)tmpBuf)[d];
        return readBlock(realBlock, buffer);
    }
    else if (block < EXT2_IND_BLOCK + TRIPLE_IND_PTR_BLOCKS + DOUBLE_IND_PTR_BLOCKS + SINGLE_IND_PTR_BLOCKS)  // Triple indirect
    {
        // TODO   
    }
    
    return false;
}

static VfsNode_t *getFsNode()
{
    VfsNode_t *rootNode = (VfsNode_t *)kmalloc(sizeof(VfsNode_t));
    if (!rootNode)
        return NULL;
    
    rootNode->inode = INODE_ROOT;
    rootNode->name[0] = '/'; rootNode->name[1] = '\0';
    rootNode->uid = g_rootInode->i_uid;
    rootNode->gid = g_rootInode->i_gid;
    rootNode->length = g_rootInode->i_size;
    rootNode->attr = g_rootInode->i_mode & 0xFFF;
    rootNode->atime = g_rootInode->i_atime;
    rootNode->mtime = g_rootInode->i_mtime;
    rootNode->ctime = g_rootInode->i_ctime;

    uint32_t mask = g_rootInode->i_mode & 0xF000;
    assert((mask & I_DIR) == I_DIR);    // Must be a directory
    assert((mask & I_FILE) != I_FILE);  // Cannot be a file

    // Set flags
    rootNode->flags = FS_DIR;
    if ((mask & I_BLKDEV) == I_BLKDEV)
        rootNode->flags |= FS_BLKDEV;
    if ((mask & I_CHARDEV) == I_CHARDEV)
        rootNode->flags |= FS_CHARDEV;
    if ((mask & I_FIFO) == I_FIFO)
        rootNode->flags |= FS_FIFO;
    if ((mask & I_SYMLINK) == I_SYMLINK)
        rootNode->flags |= FS_SYMLINK;
    
    // Set callback functions
    rootNode->read = ext2_read;
    rootNode->write = ext2_write;
    rootNode->open = ext2_open;
    rootNode->close = ext2_close;
    rootNode->readdir = ext2_readdir;
    rootNode->finddir = ext2_finddir;
    rootNode->create = ext2_create;
    rootNode->mkdir = ext2_mkdir;
    return rootNode;   
}

void ext2_init()
{
    assert(ide_read(SUPER_BLOCK_SECTOR, &g_superBlock, sizeof(SuperBlock_t) / ATA_SECTOR_SIZE));
    assert(g_superBlock.s_magic == EXT2_SIGNATURE);
    
    g_blockSize = 1024 << g_superBlock.s_log_block_size;
    g_blockGroupDescriptorCount = MAX(g_superBlock.s_blocks_count / g_superBlock.s_blocks_per_group, g_superBlock.s_inodes_count / g_superBlock.s_inodes_per_group);
    LOG("Ext2 block size: %u\n", g_blockSize);
    
    // Read group descriptors
    size_t groupBlockSpan = (g_blockGroupDescriptorCount * sizeof(BlockGroupDescriptor_t)) / g_blockSize + 1;
    g_blockGroupDescriptors = (BlockGroupDescriptor_t *)kmalloc(g_blockSize * groupBlockSpan);
    assert(g_blockGroupDescriptors);
    
    uint8_t *bufOffset = (uint8_t *)g_blockGroupDescriptors;
    for (size_t i = 0; i < groupBlockSpan; i++)
        assert(readBlock(BLOCK_GROUP_START + i, bufOffset + i * g_blockSize));
    
    // Verify group descriptors
    for (size_t i = 0; i < g_blockGroupDescriptorCount; i++)
    {
        BlockGroupDescriptor_t *group = &g_blockGroupDescriptors[i];
        assert(group->bg_inode_bitmap - group->bg_block_bitmap == 1);
        assert(group->bg_inode_table - group->bg_inode_bitmap == 1);
        
        LOG("Block group descriptor %d. Block bitmap: %u, Inode bitmap: %u, Inode table: %u, Directories: %u\n", i + 1, group->bg_block_bitmap, group->bg_inode_bitmap, group->bg_inode_table, group->bg_used_dirs_count);
    }
    
    // Read root inode
    g_rootInode = readInode(INODE_ROOT);
    assert(g_rootInode);

    // Set ext2 as root filesystem
    assert(_RootFS = getFsNode());
}

uint32_t ext2_read(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    DEF_TMP_BUF();
    if ((node->flags & FS_FILE) != FS_FILE)
        return 0;
    
    Inode_t *inode = readInode(node->inode);
    if (!inode)
        return 0;
    if (size + offset > inode->i_size)  // Cap the reading size
        size = inode->i_size - offset;
    
    uint32_t readBytes = 0, startBlock = offset / g_blockSize;
    uint32_t endBlock = (offset + size) / g_blockSize;
    if (startBlock == endBlock)
        endBlock++;

    uint8_t *pBuf = (uint8_t *)buffer;
    for (uint32_t block = startBlock; block < endBlock; block++)
    {
        // Handle offset in case of starting or ending block
        if (block == startBlock)
        {
            if (!readInodeBlock(inode, block, tmpBuf))
                goto end;
            
            uint32_t intrnOffset = offset % g_blockSize;
            uint32_t intrnCount = 0;
            if (size + intrnOffset <= g_blockSize)
                intrnCount = size;
            else
                intrnCount = g_blockSize - intrnOffset;
            
            memcpy(pBuf, tmpBuf + intrnOffset, intrnCount);
            readBytes += intrnCount;
        }
        else if (block == endBlock)
        {
            if (!readInodeBlock(inode, block, tmpBuf))
                goto end;
            
            uint32_t remaining = (offset + size) % g_blockSize;
            memcpy(pBuf + readBytes, tmpBuf, remaining);
            readBytes += remaining;
        }
        else
        {
            // Read directly into the buffer
            if (!readInodeBlock(inode, block, pBuf + readBytes))
                goto end;
            
            readBytes += g_blockSize;
        }
    }

end:
    kfree(inode);
    return readBytes;
}

uint32_t ext2_write(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    return 0;
}

void ext2_open(VfsNode_t *node, uint8_t read, uint8_t write)
{

}

void ext2_close(VfsNode_t *node)
{

}

struct dirent *ext2_readdir(VfsNode_t *node, uint32_t index)
{
    DEF_TMP_BUF();
    if ((node->flags & FS_DIR) != FS_DIR)
        return NULL;

    Inode_t *inode = readInode(node->inode);
    if (!inode)
        return NULL;
    
    // Iterate all blocks
    uint32_t i = 0;
    struct dirent *foundDir = NULL;
    for (uint32_t block = 0; block < inode->i_size / g_blockSize; block++)
    {
        if (!readInodeBlock(inode, block, tmpBuf))
            goto end;
        
        Directory_t *dir = (Directory_t *)tmpBuf;
        uint32_t currentSize = 0;
        while (true)
        {
            if (i == index)     // Found the entry
            {
                foundDir = (struct dirent *)kmalloc(sizeof(struct dirent));
                if (!foundDir)
                    goto end;
                
                foundDir->ino = dir->inode;
                strcpy(foundDir->name, dir->name);
                goto end;
            }
            
            currentSize += dir->rec_len;
            if (currentSize >= g_blockSize)
                break;

            dir = (Directory_t *)(((uint8_t *)dir) + dir->rec_len);
            i++;
        }
    }

end:
    kfree(inode);
    return foundDir;
}

VfsNode_t *ext2_finddir(VfsNode_t *node, const char *path)
{
    return NULL;
}

int ext2_create(VfsNode_t *node, const char *path, uint32_t attr)
{
    return 0;
}

int ext2_mkdir(VfsNode_t *node, const char *path, uint32_t attr)
{
    return 0;
}