#pragma once

#include <fs/vfs.h>

#define EXT2_SIGNATURE          0xEF53
#define EXT2_MAX_NAME           255

#define	EXT2_NDIR_BLOCKS		12
#define	EXT2_IND_BLOCK			EXT2_NDIR_BLOCKS
#define	EXT2_DIND_BLOCK			(EXT2_IND_BLOCK + 1)
#define	EXT2_TIND_BLOCK			(EXT2_DIND_BLOCK + 1)
#define	EXT2_N_BLOCKS			(EXT2_TIND_BLOCK + 1)

#define I_FIFO      0x1000
#define I_CHARDEV   0x2000
#define I_DIR       0x4000
#define I_BLKDEV    0x6000
#define I_FILE      0x8000
#define I_SYMLINK   0xA000
#define I_SOCK      0xC000

#define ID_UNKNOWN 0x0
#define ID_FILE    0x1
#define ID_DIR     0x2
#define ID_CHARDEV 0x3
#define ID_BLKDEV  0x4
#define ID_FIFO    0x5
#define ID_SOCK    0x6
#define ID_SYMLINK 0x7

#define EXT2_IXOTH   0x1
#define EXT2_IWOTH   0x2
#define EXT2_IROTH   0x4
#define EXT2_IRWXO   (EXT2_IXOTH | EXT2_IWOTH | EXT2_IROTH)
#define EXT2_IXGRP   0x10
#define EXT2_IWGRP   0x20
#define EXT2_IRGRP   0x40
#define EXT2_IRWXG   (EXT2_IXGRP | EXT2_IWGRP | EXT2_IRGRP)
#define EXT2_IXUSR   0x100
#define EXT2_IWUSR   0x200
#define EXT2_IRUSR   0x400
#define EXT2_IRWXU   (EXT2_IXUSR | EXT2_IWUSR | EXT2_IRUSR)

typedef struct SUPERBLOCK
{
    uint32_t	s_inodes_count;		/* Inodes count */
    uint32_t	s_blocks_count;		/* Blocks count */
    uint32_t	s_r_blocks_count;	/* Reserved blocks count */
    uint32_t	s_free_blocks_count;	/* Free blocks count */
    uint32_t	s_free_inodes_count;	/* Free inodes count */
    uint32_t	s_first_data_block;	/* First Data Block */
    uint32_t	s_log_block_size;	/* Block size */
    uint32_t	s_log_frag_size;	/* Fragment size */
    uint32_t	s_blocks_per_group;	/* # Blocks per group */
    uint32_t	s_frags_per_group;	/* # Fragments per group */
    uint32_t	s_inodes_per_group;	/* # Inodes per group */
    uint32_t	s_mtime;		/* Mount time */
    uint32_t	s_wtime;		/* Write time */
    uint16_t	s_mnt_count;		/* Mount count */
    uint16_t	s_max_mnt_count;	/* Maximal mount count */
    uint16_t	s_magic;		/* Magic signature */
    uint16_t	s_state;		/* File system state */
    uint16_t	s_errors;		/* Behaviour when detecting errors */
    uint16_t	s_minor_rev_level; 	/* minor revision level */
    uint32_t	s_lastcheck;		/* time of last check */
    uint32_t	s_checkinterval;	/* max. time between checks */
    uint32_t	s_creator_os;		/* OS */
    uint32_t	s_rev_level;		/* Revision level */
    uint16_t	s_def_resuid;		/* Default uid for reserved blocks */
    uint16_t	s_def_resgid;		/* Default gid for reserved blocks */
    /*
    * These fields are for EXT2_DYNAMIC_REV superblocks only.
    *
    * Note: the difference between the compatible feature set and
    * the incompatible feature set is that if there is a bit set
    * in the incompatible feature set that the kernel doesn't
    * know about, it should refuse to mount the filesystem.
    * 
    * e2fsck's requirements are more strict; if it doesn't know
    * about a feature in either the compatible or incompatible
    * feature set, it must abort and not try to meddle with
    * things it doesn't understand...
    */
    uint32_t	s_first_ino; 		/* First non-reserved inode */
    uint16_t    s_inode_size; 		/* size of inode structure */
    uint16_t	s_block_group_nr; 	/* block group # of this superblock */
    uint32_t	s_feature_compat; 	/* compatible feature set */
    uint32_t	s_feature_incompat; 	/* incompatible feature set */
    uint32_t	s_feature_ro_compat; 	/* readonly-compatible feature set */
    uint8_t	    s_uuid[16];		/* 128-bit uuid for volume */
    char	    s_volume_name[16]; 	/* volume name */
    char	    s_last_mounted[64]; 	/* directory where last mounted */
    uint32_t	s_algorithm_usage_bitmap; /* For compression */
    /*
    * Performance hints.  Directory preallocation should only
    * happen if the EXT2_COMPAT_PREALLOC flag is on.
    */
    uint8_t	    s_prealloc_blocks;	/* Nr of blocks to try to preallocate*/
    uint8_t	    _prealloc_dir_blocks;	/* Nr to preallocate for dirs */
    uint16_t	s_padding1;
    /*
    * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
    */
    uint8_t	    s_journal_uuid[16];	/* uuid of journal superblock */
    uint32_t	s_journal_inum;		/* inode number of journal file */
    uint32_t	s_journal_dev;		/* device number of journal file */
    uint32_t	s_last_orphan;		/* start of list of inodes to delete */
    uint32_t	s_hash_seed[4];		/* HTREE hash seed */
    uint8_t	    s_def_hash_version;	/* Default hash version to use */
    uint8_t	    s_reserved_char_pad;
    uint16_t	s_reserved_word_pad;
    uint32_t	s_default_mount_opts;
    uint32_t	s_first_meta_bg; 	/* First metablock block group */
    uint32_t	s_reserved[190];	/* Padding to the end of the block */
} __PACKED__ SuperBlock_t;

typedef struct BLOCK_GROUP_DESCRIPTOR
{
    uint32_t	bg_block_bitmap;		/* Blocks bitmap block */
    uint32_t	bg_inode_bitmap;		/* Inodes bitmap block */
    uint32_t	bg_inode_table;		/* Inodes table block */
    uint16_t	bg_free_blocks_count;	/* Free blocks count */
    uint16_t	bg_free_inodes_count;	/* Free inodes count */
    uint16_t	bg_used_dirs_count;	/* Directories count */
    uint16_t	bg_pad;
    uint32_t	bg_reserved[3];
} __PACKED__ BlockGroupDescriptor_t;

typedef struct INODE
{
    uint16_t	i_mode;		/* File mode */
    uint16_t	i_uid;		/* Low 16 bits of Owner Uid */
    uint32_t	i_size;		/* Size in bytes */
    uint32_t	i_atime;	/* Access time */
    uint32_t	i_ctime;	/* Creation time */
    uint32_t	i_mtime;	/* Modification time */
    uint32_t	i_dtime;	/* Deletion Time */
    uint16_t	i_gid;		/* Low 16 bits of Group Id */
    uint16_t	i_links_count;	/* Links count */
    uint32_t	i_sectors;	/* Sector count */
    uint32_t	i_flags;	/* File flags */
    union
    {
        struct
        {
            uint32_t  l_i_reserved1;
        } linux1;
        struct
        {
            uint32_t  h_i_translator;
        } hurd1;
        struct
        {
            uint32_t  m_i_reserved1;
        } masix1;
    } osd1;				/* OS dependent 1 */
    uint32_t	i_block[EXT2_N_BLOCKS];/* Pointers to blocks */
    uint32_t	i_generation;	/* File version (for NFS) */
    uint32_t	i_file_acl;	/* File ACL */
    uint32_t	i_dir_acl;	/* Directory ACL */
    uint32_t	i_faddr;	/* Fragment address */
    union
    {
        struct
        {
            uint8_t	    l_i_frag;	/* Fragment number */
            uint8_t	    l_i_fsize;	/* Fragment size */
            uint16_t	i_pad1;
            uint16_t	l_i_uid_high;	/* these 2 fields    */
            uint16_t	l_i_gid_high;	/* were reserved2[0] */
            uint32_t	l_i_reserved2;
        } linux2;
        struct
        {
            uint8_t	    h_i_frag;	/* Fragment number */
            uint8_t	    h_i_fsize;	/* Fragment size */
            uint16_t	h_i_mode_high;
            uint16_t	h_i_uid_high;
            uint16_t	h_i_gid_high;
            uint32_t	h_i_author;
        } hurd2;
        struct
        {
            uint8_t	    m_i_frag;	/* Fragment number */
            uint8_t	    m_i_fsize;	/* Fragment size */
            uint16_t	m_pad1;
            uint32_t	m_i_reserved2[2];
        } masix2;
    } osd2;				/* OS dependent 2 */
} __PACKED__ Inode_t;

typedef struct DIRECTORY
{
    uint32_t	inode;			/* Inode number */
    uint16_t	rec_len;		/* Directory entry length */
    uint8_t	    name_len;		/* Name length */
    uint8_t	    file_type;
    char	    name[];			/* File name, up to EXT2_NAME_LEN */
} __PACKED__ Directory_t;

/// @brief Initialize the ext2 filesystem.
void ext2_init();

/// @brief Read from a node.
/// @param node Node to read from.
/// @param offset Offset to read from.
/// @param size Amount of bytes to read.
/// @param buffer Buffer to read to.
/// @return Bytes read.
ssize_t ext2_read(VfsNode_t *node, uint32_t offset, size_t size, void *buffer);

/// @brief Write to a node.
/// @param node Node to write to.
/// @param offset Offset to write to.
/// @param size Amount of bytes to write.
/// @param buffer Buffer to write.
/// @return Bytes written.
ssize_t ext2_write(VfsNode_t *node, uint32_t offset, size_t size, void *buffer);

/// @brief Open a file.
/// @param node Node to open.
/// @param attr Attributes to open the file with.
void ext2_open(VfsNode_t *node, uint32_t attr);

/// @brief Close a file.
/// @param node Node to close.
void ext2_close(VfsNode_t *node);

/// @brief Read from a directory.
/// @param node Directory to read from.
/// @param index Index in the directory to read.
struct dirent *ext2_readdir(VfsNode_t *node, uint32_t index);

/// @brief Find an entry in a directory.
/// @param node Directory to search in.
/// @param name Name of the file to search for.
/// @return Found file, NULL, otherwise.
VfsNode_t *ext2_finddir(VfsNode_t *node, const char *name);

/// @brief Create a file.
/// @param node Parent directory node.
/// @param name Name of the file.
/// @param attr Attributes of the file.
/// @return Status of the operation.
int ext2_create(VfsNode_t *node, const char *name, uint32_t attr);

/// @brief Create a directory.
/// @param node Parent directory node.
/// @param name Name of the directory.
/// @param attr Attributes of the directory.
/// @return Status of the operation.
int ext2_mkdir(VfsNode_t *node, const char *name, uint32_t attr);

/// @brief Get the size of a file.
/// @param node File to get the size of.
/// @return Size of the file.
long ext2_ftell(VfsNode_t *node);