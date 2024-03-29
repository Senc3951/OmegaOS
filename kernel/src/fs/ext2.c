#include <fs/ext2.h>
#include <dev/storage/ide.h>
#include <mem/heap.h>
#include <assert.h>
#include <libc/string.h>
#include <logger.h>

#define SUPER_BLOCK_SECTOR  (1024 / ATA_SECTOR_SIZE)
#define BLOCK_GROUP_START   2
#define INODE_ROOT          2

#define GET_FLAGS(flags)    (flags & 0xF000)
#define INODE_FILE(ino)     ((GET_FLAGS(ino->i_mode) & I_FILE) == I_FILE)
#define INODE_DIR(ino)      ((GET_FLAGS(ino->i_mode) & I_DIR) == I_DIR)

static SuperBlock_t g_superBlock;
static BlockGroupDescriptor_t *g_blockGroupDescriptors;
static uint8_t *g_tmpBuf;
static uint32_t g_blockSize, g_blockGroupDescriptorCount, g_inodesPerGroup;

#define SECTORS_PER_BLOCK       (g_blockSize / ATA_SECTOR_SIZE)
#define INODES_PER_BLOCK        (g_blockSize / g_superBlock.s_inode_size)
#define PTR_BLOCKS_PER_BLOCK    (g_blockSize / sizeof(uint32_t))
#define SINGLE_IND_PTR_BLOCKS   (PTR_BLOCKS_PER_BLOCK)
#define DOUBLE_IND_PTR_BLOCKS   (SINGLE_IND_PTR_BLOCKS * PTR_BLOCKS_PER_BLOCK)
#define TRIPLE_IND_PTR_BLOCKS   (DOUBLE_IND_PTR_BLOCKS * PTR_BLOCKS_PER_BLOCK)
#define SECTOR2BLOCK(sector)    ((sector + 1) * ATA_SECTOR_SIZE / g_blockSize)

#define b2s(block)                  ((block) * SECTORS_PER_BLOCK)
#define readBlock(block, buffer)    (ide_read(b2s(block), buffer, SECTORS_PER_BLOCK))
#define writeBlock(block, buffer)   (ide_write(b2s(block), buffer, SECTORS_PER_BLOCK))

#define FIND_AND_MARK_BIT(bitmap, foundLabel) ({    \
    bit = 0;                                        \
    for (uint32_t k = 0; k < g_blockSize; k++) {    \
        if (bitmap[k] != UINT8_MAX) {               \
            for (uint32_t j = 0; j < 8; j++) {      \
                if (!(bitmap[k] & (1 << j))) {      \
                    bitmap[k] |= (1 << j);          \
                    bit = 8 * k + j;                \
                    goto foundLabel;                \
                }                                   \
            }                                       \
        }                                           \
    }                                               \
})

#define CLEAR_BIT(bitmap, bit) ({   \
    uint32_t x = (bit) / 8;         \
    uint32_t y = (bit) % 8;         \
    bitmap[x] &= ~(1 << y);         \
})

#define GET_DIR_ENTRY_SIZE(nl) ({               \
    uint32_t es = sizeof(Directory_t) + nl + 1; \
    es += (es % 2 == 0) ? 2 : 1;                \
    es;                                         \
})

#define INIT_INODE(inode) ({                        \
    memset(inode, 0, g_superBlock.s_inode_size);    \
    inode->i_mode = attr;                           \
    /* TODO: set a/c/m time. */                     \
})

static uint8_t getFileType(const uint32_t attr)
{
    uint32_t fileType = GET_FLAGS(attr);
    if ((fileType & I_FIFO) == I_FIFO)
        return ID_FIFO;
    else if ((fileType & I_CHARDEV) == I_CHARDEV)
        return ID_CHARDEV;
    else if ((fileType & I_DIR) == I_DIR)
        return ID_DIR;
    else if ((fileType & I_BLKDEV) == I_BLKDEV)
        return ID_BLKDEV;
    else if ((fileType & I_FILE) == I_FILE)
        return ID_FILE;
    else if ((fileType & I_SYMLINK) == I_SYMLINK)
        return ID_SYMLINK;
    else if ((fileType & I_SOCK) == I_SOCK)
        return ID_SOCK;
    
    return ID_UNKNOWN;
}

static Inode_t *readInode(const uint32_t ino)
{
    uint32_t group = (ino - 1) / g_superBlock.s_inodes_per_group;
    if (group >= g_blockGroupDescriptorCount)
        return NULL;
    
    uint32_t inodeTableStart = g_blockGroupDescriptors[group].bg_inode_table;
    uint32_t inodeIndex = (ino - 1) % g_superBlock.s_inodes_per_group;
    uint32_t block = inodeTableStart + inodeIndex / INODES_PER_BLOCK;
    if (!readBlock(block, g_tmpBuf))
        return NULL;
    
    Inode_t *inode = (Inode_t *)kmalloc(g_superBlock.s_inode_size);
    if (!inode)
        return NULL;
    
    uint32_t blockOffset = (inodeIndex % INODES_PER_BLOCK) * g_superBlock.s_inode_size;
    memcpy(inode, g_tmpBuf + blockOffset, g_superBlock.s_inode_size);
        
    return inode;
}

static bool writeInode(const uint32_t ino, Inode_t *inode)
{
    uint32_t group = (ino - 1) / g_superBlock.s_inodes_per_group;
    if (group >= g_blockGroupDescriptorCount)
        return NULL;
    
    uint32_t inodeTableStart = g_blockGroupDescriptors[group].bg_inode_table;
    uint32_t inodeIndex = (ino - 1) % g_superBlock.s_inodes_per_group;
    uint32_t block = inodeTableStart + inodeIndex / INODES_PER_BLOCK;
    if (!readBlock(block, g_tmpBuf))
        return NULL;
    
    uint32_t blockOffset = (inodeIndex % INODES_PER_BLOCK) * g_superBlock.s_inode_size;
    memcpy(g_tmpBuf + blockOffset, inode, g_superBlock.s_inode_size);
    
    return writeBlock(block, g_tmpBuf);
}

static uint32_t getRealBlock(Inode_t *inode, const uint32_t block)
{    
    if (block < EXT2_NDIR_BLOCKS)   // Direct
        return inode->i_block[block];
    else if (block < EXT2_IND_BLOCK + SINGLE_IND_PTR_BLOCKS)   // Single indirect
    {
        // Read single indirect block
        if (!readBlock(inode->i_block[EXT2_IND_BLOCK], g_tmpBuf))
            return 0;
        
        return ((uint32_t *)g_tmpBuf)[block - EXT2_IND_BLOCK];
    }
    else if (block < EXT2_IND_BLOCK + DOUBLE_IND_PTR_BLOCKS + SINGLE_IND_PTR_BLOCKS)    // Double indirect
    {
        // Read double indirect block
        if (!readBlock(inode->i_block[EXT2_DIND_BLOCK], g_tmpBuf))
            return 0;

        uint32_t b = (block - EXT2_DIND_BLOCK) / PTR_BLOCKS_PER_BLOCK;
        uint32_t c = b / PTR_BLOCKS_PER_BLOCK;
        uint32_t d = b - c * PTR_BLOCKS_PER_BLOCK;
        
        // Read single indirect block
        uint32_t singleIndirectBlock = ((uint32_t *)g_tmpBuf)[c];
        if (!readBlock(singleIndirectBlock, g_tmpBuf))
            return 0;
        
        return ((uint32_t *)g_tmpBuf)[d];
    }
    else if (block < EXT2_IND_BLOCK + TRIPLE_IND_PTR_BLOCKS + DOUBLE_IND_PTR_BLOCKS + SINGLE_IND_PTR_BLOCKS)  // Triple indirect
    {
        // Read triple indirect block
        if (!readBlock(inode->i_block[EXT2_TIND_BLOCK], g_tmpBuf))
            return 0;
        
        uint32_t b = block - EXT2_TIND_BLOCK - PTR_BLOCKS_PER_BLOCK;
        uint32_t c = b - PTR_BLOCKS_PER_BLOCK * PTR_BLOCKS_PER_BLOCK;
        uint32_t d = c / (PTR_BLOCKS_PER_BLOCK * PTR_BLOCKS_PER_BLOCK);
        uint32_t e = c - d * PTR_BLOCKS_PER_BLOCK * PTR_BLOCKS_PER_BLOCK;
        uint32_t f = e / PTR_BLOCKS_PER_BLOCK;
        uint32_t g = e - f * PTR_BLOCKS_PER_BLOCK;

        // Read double indirect block
        uint32_t doubleIndirectBlock = ((uint32_t *)g_tmpBuf)[d];
        if (!readBlock(doubleIndirectBlock, g_tmpBuf))
            return 0;

        // Read single indirect block
        uint32_t singleIndirectBlock = ((uint32_t *)g_tmpBuf)[f];
        if (!readBlock(singleIndirectBlock, g_tmpBuf))
            return 0;
        
        return ((uint32_t *)g_tmpBuf)[g];
    }
    
    return 0;
}

static bool setRealBlock(Inode_t *inode, const uint32_t block, const uint32_t real)
{
    if (block < EXT2_NDIR_BLOCKS)   // Direct
    {
        inode->i_block[block] = real;
        return true;
    }
    else if (block < EXT2_IND_BLOCK + SINGLE_IND_PTR_BLOCKS)   // Single indirect
    {
        // Read single indirect block
        if (!readBlock(inode->i_block[EXT2_IND_BLOCK], g_tmpBuf))
            return false;
        
        ((uint32_t *)g_tmpBuf)[block - EXT2_IND_BLOCK] = real;
        return true;
    }
    else if (block < EXT2_IND_BLOCK + DOUBLE_IND_PTR_BLOCKS + SINGLE_IND_PTR_BLOCKS)    // Double indirect
    {
        // Read double indirect block
        if (!readBlock(inode->i_block[EXT2_DIND_BLOCK], g_tmpBuf))
            return false;
        
        uint32_t b = (block - EXT2_DIND_BLOCK) / PTR_BLOCKS_PER_BLOCK;
        uint32_t c = b / PTR_BLOCKS_PER_BLOCK;
        uint32_t d = b - c * PTR_BLOCKS_PER_BLOCK;
        
        // Read single indirect block
        uint32_t singleIndirectBlock = ((uint32_t *)g_tmpBuf)[c];
        if (!readBlock(singleIndirectBlock, g_tmpBuf))
            return false;
        
        ((uint32_t *)g_tmpBuf)[d] = real;
        return true;
    }
    else if (block < EXT2_IND_BLOCK + TRIPLE_IND_PTR_BLOCKS + DOUBLE_IND_PTR_BLOCKS + SINGLE_IND_PTR_BLOCKS)  // Triple indirect
    {
        // Read triple indirect block
        if (!readBlock(inode->i_block[EXT2_TIND_BLOCK], g_tmpBuf))
            return false;
        
        uint32_t b = block - EXT2_TIND_BLOCK - PTR_BLOCKS_PER_BLOCK;
        uint32_t c = b - PTR_BLOCKS_PER_BLOCK * PTR_BLOCKS_PER_BLOCK;
        uint32_t d = c / (PTR_BLOCKS_PER_BLOCK * PTR_BLOCKS_PER_BLOCK);
        uint32_t e = c - d * PTR_BLOCKS_PER_BLOCK * PTR_BLOCKS_PER_BLOCK;
        uint32_t f = e / PTR_BLOCKS_PER_BLOCK;
        uint32_t g = e - f * PTR_BLOCKS_PER_BLOCK;

        // Read double indirect block
        uint32_t doubleIndirectBlock = ((uint32_t *)g_tmpBuf)[d];
        if (!readBlock(doubleIndirectBlock, g_tmpBuf))
            return false;

        // Read single indirect block
        uint32_t singleIndirectBlock = ((uint32_t *)g_tmpBuf)[f];
        if (!readBlock(singleIndirectBlock, g_tmpBuf))
            return false;
        
        ((uint32_t *)g_tmpBuf)[g] = real;
        return true;
    }
    
    return false;
}

static bool allocateBlock(Inode_t *inode, const uint32_t ino, uint32_t block)
{
    uint8_t *blockBitmap = (uint8_t *)kmalloc(g_blockSize);
    if (!blockBitmap)
        return false;
    
    uint32_t bit = 0, newBlock = 0;
    for (uint32_t i = 0; i < g_blockGroupDescriptorCount; i++)
    {
        if (g_blockGroupDescriptors[i].bg_free_blocks_count > 0)
        {
            if (!readBlock(g_blockGroupDescriptors[i].bg_block_bitmap, blockBitmap))
            {
                kfree(blockBitmap);   
                return NULL;
            }
            
            FIND_AND_MARK_BIT(blockBitmap, found_bit);    // Jump if found a bit
            continue;   // Continue to the next group descriptor
        }
          
found_bit:
        if (!bit)
        {
            kfree(blockBitmap);
            return NULL;
        }
        
        newBlock = bit + g_superBlock.s_blocks_per_group * i + 1;        
        g_blockGroupDescriptors[i].bg_free_blocks_count--;
        writeBlock(g_blockGroupDescriptors[i].bg_block_bitmap, blockBitmap);
        writeBlock(BLOCK_GROUP_START, g_blockGroupDescriptors);
        
        break;
    }

    kfree(blockBitmap);
    if (!newBlock)
        return false;
    
    // Write block
    if (!setRealBlock(inode, block, newBlock))
        return false;
    
    inode->i_sectors += SECTORS_PER_BLOCK;  // Update sector count
    inode->i_size += g_blockSize;   // Update size
    writeInode(ino, inode);
    
    return true;
}

static bool readInodeBlock(Inode_t *inode, const uint32_t block, void *buffer)
{
    return readBlock(getRealBlock(inode, block), buffer);
}

static bool writeInodeBlock(Inode_t *inode, const uint32_t ino, const uint32_t block, void *buffer)
{
    while (block >= SECTOR2BLOCK(inode->i_sectors))    // Allocate blocks
    {
        if (!allocateBlock(inode, ino, SECTOR2BLOCK(inode->i_sectors)))
            return false;
    }
    
    return writeBlock(getRealBlock(inode, block), buffer);
}

static bool insertInodeInDir(Inode_t *parentInode, uint32_t pino, const uint32_t newIno, const char *name, const uint8_t ft)
{
    uint8_t *tmpBuf = (uint8_t *)kmalloc(g_blockSize);
    if (!tmpBuf)
        return false;

    uint32_t lastBlock = parentInode->i_size / g_blockSize - 1;
    if (!readInodeBlock(parentInode, lastBlock, tmpBuf))
    {
        kfree(tmpBuf);
        return false;
    }
    
    // Find the last entry
    Directory_t *dir = (Directory_t *)tmpBuf;
    uint32_t oldSize, currentEntrySize, currentSize = 0, newEntrySize = GET_DIR_ENTRY_SIZE(strlen(name));
    
    // If no blocks exist, the subtrcation will overflow, so allocate a new block
    while (lastBlock != UINT32_MAX)
    {
        currentSize += dir->rec_len;
        if (currentSize >= g_blockSize)
        {
            // Check if the last entry is big enough to hold another entry
            oldSize = dir->rec_len, currentEntrySize = GET_DIR_ENTRY_SIZE(dir->name_len);
            if (currentEntrySize + newEntrySize <= dir->rec_len)
            {
                dir->rec_len = currentEntrySize;
                dir = (Directory_t *)(((uint8_t *)dir) + dir->rec_len);
                
                // Write the new entry
                goto write;
            }
            
            // No more place in the current block
            break;
        }
        
        dir = (Directory_t *)(((uint8_t *)dir) + dir->rec_len);
    }
    
    // Allocate a new block
    lastBlock++;
    if (!allocateBlock(parentInode, pino, lastBlock))
    {
        kfree(tmpBuf);
        return false;
    }
    if (!readInodeBlock(parentInode, lastBlock, tmpBuf))
    {
        kfree(tmpBuf);
        return false;
    }
    
    dir = (Directory_t *)tmpBuf;
    oldSize = g_blockSize;
    currentEntrySize = 0;
    
write:
    dir->inode = newIno;
    strcpy(dir->name, name);
    dir->name_len = strlen(name);
    dir->rec_len = oldSize - currentEntrySize;
    dir->file_type = ft;
    
    bool ret = writeInodeBlock(parentInode, pino, lastBlock, tmpBuf);
    kfree(tmpBuf);
    return ret;
}

static int deleteBlockBuffer(uint8_t *bitmap, uint32_t block)
{
    if (!block)
        return EIO;
    
    uint32_t blockIndex = block / g_superBlock.s_blocks_per_group;
    uint32_t blockOffset = block % g_superBlock.s_blocks_per_group;
    if (blockIndex >= g_blockGroupDescriptorCount)
        return EPERM;

    // Read the block bitmap
    if (!readBlock(g_blockGroupDescriptors[blockIndex].bg_block_bitmap, bitmap))
        return EIO;
    
    CLEAR_BIT(bitmap, blockOffset - 1);
    g_blockGroupDescriptors[blockIndex].bg_free_blocks_count++;
    writeBlock(g_blockGroupDescriptors[blockIndex].bg_block_bitmap, bitmap);
    writeBlock(BLOCK_GROUP_START, g_blockGroupDescriptors);

    return ENOER;
}

static int deleteBlock(uint32_t block)
{
    uint8_t *blockBitmap = (uint8_t *)kmalloc(g_blockSize);
    if (!blockBitmap)
        return ENOMEM;
    
    int ret = deleteBlockBuffer(blockBitmap, block);
    kfree(blockBitmap);

    return ret;
}

static int deleteInodeFromDir(Inode_t *parentInode, uint32_t pino, const char *name, uint32_t *ino)
{
    if (!strcmp(name, FS_PATH_CURR_DIR) || !strcmp(name, FS_PATH_UP_DIR))
        return EACCES;
    
    int ret = ENOENT;
    uint8_t *tmpBuf = (uint8_t *)kmalloc(g_blockSize);
    if (!tmpBuf)
        return ENOMEM;

    // Iterate all blocks
    Directory_t *dir = NULL, *prev = NULL;
    uint32_t block = 0, currentSize = 0;
    for (; block < parentInode->i_size / g_blockSize; block++)
    {
        if (!readInodeBlock(parentInode, block, tmpBuf))
            goto end;
        
        dir = (Directory_t *)tmpBuf, prev = NULL;
        currentSize = 0;
        while (true)
        {
            if (!strcmp(dir->name, name))     // Found the entry
            {
                // Delete the entry
                *ino = dir->inode;
                if (currentSize == 0 && dir->rec_len >= g_blockSize)  // First and only entry
                {
                    // Free the current block
                    ret = deleteBlock(getRealBlock(parentInode, block));
                    if (ret != ENOER)
                        goto end;
                    
                    // Update inode
                    parentInode->i_sectors -= SECTORS_PER_BLOCK;
                    parentInode->i_size -= g_blockSize;
                    
                    ret = writeInode(pino, parentInode) ? ENOER : EIO;
                    goto end;
                }
                else if (currentSize + dir->rec_len >= g_blockSize)  // Last entry
                {
                    prev->rec_len += dir->rec_len;
                    ret = writeInodeBlock(parentInode, pino, block, tmpBuf) ? ENOER : EIO;

                    goto end;
                }
                
                // File is either in the middle or in the beginning with more files in the current block
                goto delete_middle_entry;                
            }
            
            currentSize += dir->rec_len;
            if (currentSize >= g_blockSize)
                goto end;
            
            prev = dir;
            dir = (Directory_t *)(((uint8_t *)dir) + dir->rec_len);
        }
    }

delete_middle_entry:
    uint32_t feLen = dir->rec_len;
    memmove(tmpBuf + currentSize, tmpBuf + currentSize + feLen, g_blockSize - feLen - currentSize);   // Copy to the left
    
    // Find the last entry
    Directory_t *tmpDir = (Directory_t *)tmpBuf;
    uint32_t csTmp = currentSize + feLen;
    while (true)
    {
        csTmp += tmpDir->rec_len;
        if (csTmp >= g_blockSize)
            break;

        tmpDir = (Directory_t *)(((uint8_t *)tmpDir) + tmpDir->rec_len);
    }
    
    tmpDir->rec_len += feLen;   // Update the size
    ret = writeInodeBlock(parentInode, pino, block, tmpBuf) ? ENOER : EIO;

end:
    kfree(tmpBuf);
    return ret;
}

static int deleteInode(Inode_t *inode, uint32_t ino)
{
    uint8_t *generalBitmap = (uint8_t *)kmalloc(g_blockSize);
    if (!generalBitmap)
        return ENOMEM;
    
    // Delete Inode
    uint32_t inodeIndex = ino / g_inodesPerGroup;
    uint32_t inodeOffset = ino % g_inodesPerGroup;
    if (inodeIndex >= g_blockGroupDescriptorCount)
    {
        kfree(generalBitmap);
        return EPERM;
    }
    
    // Read the inode bitmap
    if (!readBlock(g_blockGroupDescriptors[inodeIndex].bg_inode_bitmap, generalBitmap))
    {
        kfree(generalBitmap);
        return EIO;
    }
    
    CLEAR_BIT(generalBitmap, inodeOffset - 1);
    g_blockGroupDescriptors[inodeIndex].bg_free_inodes_count++;
    writeBlock(g_blockGroupDescriptors[inodeIndex].bg_inode_bitmap, generalBitmap);
    writeBlock(BLOCK_GROUP_START, g_blockGroupDescriptors);
    
    if (inode->i_size == 0)
    {
        kfree(generalBitmap);
        return ENOER;
    }
    
    // Delete data blocks
    uint32_t endBlock = inode->i_size / g_blockSize;
    if (inode->i_size % g_blockSize != 0)
        endBlock++;
    
    for (uint32_t block = 0; block < endBlock; block++)
    {
        int ret = deleteBlockBuffer(generalBitmap, getRealBlock(inode, block));
        if (ret != ENOER)
        {
            kfree(generalBitmap);
            return ret;
        }
    }

    kfree(generalBitmap);
    return ENOER;
}

static Inode_t *createInode(Inode_t *parentInode, const uint32_t pino, const char *name, uint32_t attr, uint32_t ino)
{
    Inode_t *inode = readInode(ino);
    if (!inode)
        return NULL;
    
    // Write the new inode
    INIT_INODE(inode);
    if (!writeInode(ino, inode))
    {
        kfree(inode);
        return NULL;
    }

    if (!insertInodeInDir(parentInode, pino, ino, name, getFileType(attr)))
    {
        kfree(inode);
        return NULL;
    }
       
    return inode;
}

static Inode_t *allocateAndCreateInode(Inode_t *parentInode, const uint32_t pino, const char *name, uint32_t attr, uint32_t *newIno)
{
    uint8_t *inodeBitmap = (uint8_t *)kmalloc(g_blockSize);
    if (!inodeBitmap)
        return NULL;
    
    uint32_t bit = 0, newInoNum = 0;
    for (uint32_t i = 0; i < g_blockGroupDescriptorCount; i++)
    {
        if (g_blockGroupDescriptors[i].bg_free_inodes_count > 0)
        {
            if (!readBlock(g_blockGroupDescriptors[i].bg_inode_bitmap, inodeBitmap))
            {
                kfree(inodeBitmap);   
                return NULL;
            }
            
            FIND_AND_MARK_BIT(inodeBitmap, found_bit);    // Jump if found a bit
            continue;   // Continue to the next group descriptor
        }
        
found_bit:
        if (!bit)
        {
            kfree(inodeBitmap);
            return NULL;
        }
        
        newInoNum = bit + g_inodesPerGroup * i + 1;
        g_blockGroupDescriptors[i].bg_free_inodes_count--;
        writeBlock(g_blockGroupDescriptors[i].bg_inode_bitmap, inodeBitmap);
        writeBlock(BLOCK_GROUP_START, g_blockGroupDescriptors);
        
        break;
    }

    kfree(inodeBitmap);
    if (!newInoNum)
        return NULL;
    
    *newIno = newInoNum;
    return createInode(parentInode, pino, name, attr, newInoNum);
}

static VfsNode_t *ino2vfs(const uint32_t ino, const char *name)
{
    Inode_t *inode = readInode(ino);
    if (!inode)
        return NULL;
        
    VfsNode_t *node = (VfsNode_t *)kmalloc(sizeof(VfsNode_t));
    if (!node)
    {
        kfree(inode);
        return NULL;
    }
    
    node->inode = ino;
    strcpy(node->name, name);
    node->uid = inode->i_uid;
    node->gid = inode->i_gid;
    node->size = inode->i_size;
    node->mode = inode->i_mode & 0xFFF;
    node->atime = inode->i_atime;
    node->mtime = inode->i_mtime;
    node->ctime = inode->i_ctime;
    node->offset = 0;
    
    // Set flags
    uint32_t mask = GET_FLAGS(inode->i_mode);
    if ((mask & I_FILE) == I_FILE)
    {
        node->flags |= FS_FILE;
        node->readdir = NULL;
        node->finddir = NULL;
        node->create = NULL;
        node->mkdir = NULL;
        node->delete = NULL;
    }
    if ((mask & I_DIR) == I_DIR)
    {
        node->flags |= FS_DIR;
        node->readdir = ext2_readdir;
        node->finddir = ext2_finddir;
        node->create = ext2_create;
        node->mkdir = ext2_mkdir;
        node->delete = ext2_delete;
    }
    if ((mask & I_BLKDEV) == I_BLKDEV)
        node->flags |= FS_BLKDEV;
    if ((mask & I_CHARDEV) == I_CHARDEV)
        node->flags |= FS_CHARDEV;
    if ((mask & I_FIFO) == I_FIFO)
        node->flags |= FS_FIFO;
    if ((mask & I_SYMLINK) == I_SYMLINK)
        node->flags |= FS_SYMLINK;
    
    // Set callback functions
    node->read = ext2_read;
    node->write = ext2_write;
    node->open = ext2_open;
    node->close = ext2_close;
    
    kfree(inode);
    return node;
}

void ext2_init()
{
    assert(ide_read(SUPER_BLOCK_SECTOR, &g_superBlock, sizeof(SuperBlock_t) / ATA_SECTOR_SIZE));
    assert(g_superBlock.s_magic == EXT2_SIGNATURE);
    
    g_blockSize = 1024 << g_superBlock.s_log_block_size;
    g_blockGroupDescriptorCount = MAX(g_superBlock.s_blocks_count / g_superBlock.s_blocks_per_group, g_superBlock.s_inodes_count / g_superBlock.s_inodes_per_group);
    g_inodesPerGroup = g_superBlock.s_inodes_count / g_blockGroupDescriptorCount;
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
    
    assert(g_tmpBuf = (uint8_t *)kmalloc(g_blockSize));

    // Set ext2 as root filesystem
    assert(_RootFS = ino2vfs(INODE_ROOT, "/"));
    assert((_RootFS->flags & FS_DIR) == FS_DIR);
    LOG("Ext2 filesystem mounted at /\n");
}

ssize_t ext2_read(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    Inode_t *inode = readInode(node->inode);
    if (!inode)
        return -EEXIST;
    if (!INODE_FILE(inode))
    {
        kfree(inode);
        return -EISDIR;
    }
    if (size + offset > inode->i_size)
    {
        kfree(inode);
        return -ESPIPE;
    }
    
    uint8_t *tmpBuf = (uint8_t *)kmalloc(g_blockSize);
    if (!tmpBuf)
    {
        kfree(inode);
        return -ENOMEM;
    }
    
    ssize_t readBytes = 0;
    uint32_t startBlock = offset / g_blockSize;
    uint32_t endBlock = (offset + size) / g_blockSize;
    
    uint8_t *pBuf = (uint8_t *)buffer;
    for (uint32_t block = startBlock; block <= endBlock; block++)
    {
        // Handle offset in case of starting or ending block
        if (block == startBlock)
        {
            if (!readInodeBlock(inode, block, tmpBuf))
            {
                readBytes = -EIO;
                goto end;
            }
            
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
            {
                readBytes = -EIO;
                goto end;
            }
            
            uint32_t remainingBytes = (offset + size) % g_blockSize;
            memcpy(pBuf + readBytes, tmpBuf, remainingBytes);
            readBytes += remainingBytes;
        }
        else
        {
            // Read directly into the buffer
            if (!readInodeBlock(inode, block, pBuf + readBytes))
            {
                readBytes = -EIO;
                goto end;
            }

            readBytes += g_blockSize;
        }
    }
    
    node->offset = offset + readBytes;  // Update offset

end:
    kfree(inode);
    kfree(tmpBuf);
    return readBytes;
}

ssize_t ext2_write(VfsNode_t *node, uint32_t offset, size_t size, void *buffer)
{
    Inode_t *inode = readInode(node->inode);
    uint32_t osize = inode->i_size;
    if (!inode)
        return -EEXIST;
    if (!INODE_FILE(inode))
    {
        kfree(inode);
        return -EISDIR;
    }
    if (offset > inode->i_size)
    {
        kfree(inode);
        return -ESPIPE;
    }

    uint8_t *tmpBuf = (uint8_t *)kmalloc(g_blockSize);
    if (!tmpBuf)
    {
        kfree(inode);
        return -ENOMEM;
    }
    
    ssize_t writtenBytes = 0;
    uint32_t startBlock = offset / g_blockSize;
    uint32_t endBlock = (offset + size) / g_blockSize;
    
    uint8_t *pBuf = (uint8_t *)buffer;
    for (uint32_t block = startBlock; block <= endBlock; block++)
    {
        // Handle offset in case of starting or ending block
        if (block == startBlock)
        {
            if (!readInodeBlock(inode, block, tmpBuf))
            {
                writtenBytes = -EIO;
                goto end;
            }
            
            uint32_t intrnOffset = offset % g_blockSize;
            uint32_t intrnCount = 0;
            if (size + intrnOffset <= g_blockSize)
                intrnCount = size;
            else
                intrnCount = g_blockSize - intrnOffset;
            
            memcpy(tmpBuf + intrnOffset, pBuf, intrnCount);
            if (!writeInodeBlock(inode, node->inode, block, tmpBuf))
            {
                writtenBytes = -EIO;
                goto end;
            }
            
            writtenBytes += intrnCount;
        }
        else if (block == endBlock)
        {
            if (!readInodeBlock(inode, block, tmpBuf))
            {
                writtenBytes = -EIO;
                goto end;
            }
            
            uint32_t remainingBytes = (offset + size) % g_blockSize;
            memcpy(tmpBuf, pBuf + writtenBytes, remainingBytes);
            if (!writeInodeBlock(inode, node->inode, block, tmpBuf))
            {
                writtenBytes = -EIO;
                goto end;
            }
            
            writtenBytes += remainingBytes;
        }
        else
        {
            // Write directly from the buffer
            if (!writeInodeBlock(inode, node->inode, block, pBuf + writtenBytes))
            {
                writtenBytes = -EIO;
                goto end;
            }
            
            writtenBytes += g_blockSize;
        }
    }

end:
    if (!writtenBytes)
        goto cleanup;
    
    // Update inode
    if (offset + writtenBytes > osize)
    {
        inode->i_size = offset + writtenBytes;
        inode->i_sectors = RNDUP(inode->i_size, ATA_SECTOR_SIZE) / ATA_SECTOR_SIZE;
    }
    
    if (!writeInode(node->inode, inode))
    {
        writtenBytes = -EIO;
        goto cleanup;
    }
    
    // Update VfsNode
    node->size = inode->i_size;
    node->offset = offset + writtenBytes;
    
cleanup:
    kfree(inode);
    kfree(tmpBuf);
    return writtenBytes;
}

void ext2_open(VfsNode_t *node, uint32_t attr)
{
    UNUSED(node);
    UNUSED(attr);
}

void ext2_close(VfsNode_t *node)
{
    UNUSED(node);
}

struct dirent *ext2_readdir(VfsNode_t *node, uint32_t index)
{
    Inode_t *inode = readInode(node->inode);
    if (!inode)
        return NULL;
    if (!INODE_DIR(inode))
    {
        kfree(inode);
        return NULL;
    }
    
    uint8_t *tmpBuf = (uint8_t *)kmalloc(g_blockSize);
    if (!tmpBuf)
    {
        kfree(inode);
        return NULL;
    }
    
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
            
            i++;
            currentSize += dir->rec_len;
            if (currentSize >= g_blockSize)
                break;

            dir = (Directory_t *)(((uint8_t *)dir) + dir->rec_len);
        }
    }

end:
    kfree(inode);
    kfree(tmpBuf);
    return foundDir;
}

VfsNode_t *ext2_finddir(VfsNode_t *node, const char *name)
{
    Inode_t *inode = readInode(node->inode);
    if (!inode)
        return NULL;
    if (!INODE_DIR(inode))
    {
        kfree(inode);
        return NULL;
    }
    
    uint8_t *tmpBuf = (uint8_t *)kmalloc(g_blockSize);
    if (!tmpBuf)
    {
        kfree(inode);
        return NULL;
    }

    // Iterate all blocks
    VfsNode_t *foundNode = NULL;
    for (uint32_t block = 0; block < inode->i_size / g_blockSize; block++)
    {
        if (!readInodeBlock(inode, block, tmpBuf))
            goto end;
        
        Directory_t *dir = (Directory_t *)tmpBuf;
        uint32_t currentSize = 0;
        while (true)
        {
            if (!strcmp(dir->name, name))     // Found the entry
            {
                foundNode = ino2vfs(dir->inode, dir->name);
                goto end;
            }
            
            currentSize += dir->rec_len;
            if (currentSize >= g_blockSize)
                break;

            dir = (Directory_t *)(((uint8_t *)dir) + dir->rec_len);
        }
    }

end:
    kfree(inode);
    kfree(tmpBuf);
    return foundNode;
}

int ext2_create(VfsNode_t *node, const char *name, uint32_t attr)
{
    VfsNode_t *fileNode = ext2_finddir(node, name);
    if (fileNode)   // Check if the file already exists
    {
        kfree(fileNode);
        return EEXIST;
    }
    
    Inode_t *parentInode = readInode(node->inode);
    if (!parentInode)
        return ENOENT;
    if (!INODE_DIR(parentInode))
    {
        kfree(parentInode);
        return ENOTDIR;
    }
    
    // Create the new file
    uint32_t newIno;
    Inode_t *newInode = allocateAndCreateInode(parentInode, node->inode, name, (attr & ~I_DIR) | I_FILE, &newIno);
    kfree(parentInode);
    if (!newInode)
        return EPERM;
    
    kfree(newInode);
    return ENOER;
}

int ext2_mkdir(VfsNode_t *node, const char *name, uint32_t attr)
{
    VfsNode_t *fileNode = ext2_finddir(node, name);
    if (fileNode)   // Check if the directory already exists
    {
        kfree(fileNode);
        return EEXIST;
    }
    
    Inode_t *parentInode = readInode(node->inode);
    if (!parentInode)
        return ENOENT;
    if (!INODE_DIR(parentInode))
        return ENOTDIR;
    
    // Create the new file
    uint32_t newIno, eino;
    Inode_t *newInode = allocateAndCreateInode(parentInode, node->inode, name, (attr & ~I_FILE) | I_DIR, &newIno);
    kfree(parentInode);
    if (!newInode)
        return EPERM;
    
    // Create current directory pointer (inode - current directory)
    Inode_t *currentDirectoryInode = allocateAndCreateInode(newInode, newIno, FS_PATH_CURR_DIR, I_FILE, &eino);
    if (!currentDirectoryInode)
    {
        kfree(currentDirectoryInode);
        return EPERM;
    }
    kfree(currentDirectoryInode);
    
    // Create parent directory pointer (inode - parent directory)
    Inode_t *previousDirectoryInode = allocateAndCreateInode(newInode, newIno, FS_PATH_UP_DIR, I_FILE, &eino);
    if (!previousDirectoryInode)
    {
        kfree(previousDirectoryInode);
        return EPERM;
    }
    
    kfree(previousDirectoryInode);
    kfree(newInode);
    return ENOER;
}

int ext2_delete(VfsNode_t *node, const char *name)
{
    Inode_t *parentInode = readInode(node->inode);
    if (!parentInode)
        return ENOENT;
    if (!INODE_DIR(parentInode))
    {
        kfree(parentInode);
        return ENOTDIR;
    }
    
    // Delete the inode from the directory
    uint32_t cino = 0;
    int ret = deleteInodeFromDir(parentInode, node->inode, name, &cino);
    if (ret != ENOER)
    {
        kfree(parentInode);
        return ret;
    }
    kfree(parentInode);
    
    Inode_t *cInode = readInode(cino);
    if (!cInode)
        return ENOENT;
    if (INODE_DIR(cInode))
    {
        kfree(cInode);
        return EISDIR;
    }
    
    // Delete the inode
    ret = deleteInode(cInode, cino);
    kfree(cInode);
    
    return ret;
}