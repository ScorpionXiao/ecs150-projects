#ifndef __ufs_h__
#define __ufs_h__

#define UFS_DIRECTORY (0)
#define UFS_REGULAR_FILE (1)

#define UFS_ROOT_DIRECTORY_INODE_NUMBER (0)

#define UFS_BLOCK_SIZE (4096)

#define DIRECT_PTRS (30)

#define MAX_FILE_SIZE (DIRECT_PTRS * UFS_BLOCK_SIZE)

// Note: Bitmap indexes identify disk blocks relative to the start of a region.

typedef struct {
    int type;   // UFS_DIRECTORY or UFS_REGULAR
    int size;   // bytes, indicating the number of entries inside of this inode entriesNum = inode.size / sizeof(dir_ent_t)
    unsigned int direct[DIRECT_PTRS]; // pointers to blocks
} inode_t; // each inode_t is 128 bytes?

#define DIR_ENT_NAME_SIZE (28)
typedef struct {
    char name[DIR_ENT_NAME_SIZE];  // up to 28 bytes of name in directory (including \0) entry name
    int  inum;      // inode number of entry
} dir_ent_t; // each dir_ent_t is 32 bytes?

// presumed: block 0 is the super block
typedef struct __super {
    int inode_bitmap_addr; // block address (in blocks)
    int inode_bitmap_len;  // in blocks
    int data_bitmap_addr;  // block address (in blocks)
    int data_bitmap_len;   // in blocks
    int inode_region_addr; // block address (in blocks)
    int inode_region_len;  // in blocks
    int data_region_addr;  // block address (in blocks)
    int data_region_len;   // in blocks
    int num_inodes;        // just the number of inodes
    int num_data;          // and data blocks...
} super_t;


#endif // __ufs_h__
