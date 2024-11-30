#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <cstring>
#include <algorithm>
#include <stdlib.h>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

/*
read(), write(), seek()


mmap(): create mapping btw file's content and memory
munmap(): destroy mapping


super block: read only structure that describes the file system
inode: holds the metadata for a file
File data: in general an array of bytes stored in disk block  
    Directories use file data with structure, 
    they store anarray of dir_ent_t structures to describe the entires in the directory

Inode table: an array that holds all of the inodes.
Inode and data bitmaps: track allocation for blocks in the inode and the data region, respectively
Data region

inode_t: each inode is 128 bytes




for debug: gdbserver localhost:1234 ./ds3mkdir tests/disk_images/a.img 0 hi.txt

*/

LocalFileSystem::LocalFileSystem(Disk *disk) {
  this->disk = disk;
}

void LocalFileSystem::readSuperBlock(super_t *super) {
  char *buffer = new char[UFS_BLOCK_SIZE];
  disk->readBlock(0, buffer);
  memcpy(super, buffer, sizeof(super_t));
  delete[] buffer;
}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  for (int i = 0; i < super->inode_bitmap_len; i++) {
    disk->readBlock(super->inode_bitmap_addr + i, inodeBitmap + i * UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  for (int i = 0; i < super->data_bitmap_len; i++) {
    disk->readBlock(super->data_bitmap_addr + i, dataBitmap + i * UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  int inodesPerBlock = UFS_BLOCK_SIZE / sizeof(inode_t);
  for (int i = 0; i < super->inode_region_len; i++) {
    disk->readBlock(super->inode_region_addr + i, inodes + (i * inodesPerBlock));
  }
}





void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
  for (int i = 0; i < super->inode_bitmap_len; i++) {
    disk->writeBlock(super->inode_bitmap_addr + i, inodeBitmap + (i * UFS_BLOCK_SIZE));
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
  for (int i = 0; i < super->data_bitmap_len; i++) {
    disk->writeBlock(super->data_bitmap_addr + i, dataBitmap + (i * UFS_BLOCK_SIZE));
  }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
  int inodesPerBlock = UFS_BLOCK_SIZE / sizeof(inode_t);
  for (int i = 0; i < super->inode_region_len; i++) {
    disk->writeBlock(super->inode_region_addr + i, inodes + (i * inodesPerBlock));
  }
}

// void readDataRegion(super_t *super, inode_t *inodes) {
//   int data_len = super->data_region_len;
//   int data_addr = super->data_region_addr;
  
  
// }

/**
   * Lookup an inode.
   *
   * Takes the parent inode number (which should be the inode number
   * of a directory) and looks up the entry name in it. The inode
   * number of name is returned.
   *
   * Success: return inode number of name
   * Failure: return -ENOTFOUND, -EINVALIDINODE.
   * Failure modes: invalid parentInodeNumber, name does not exist.
   */
int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  super_t super;
  readSuperBlock(&super);

  inode_t parentInode;
  if (stat(parentInodeNumber, &parentInode) < 0) {
    return -EINVALIDINODE;
  }

  if (parentInode.type != UFS_DIRECTORY) {
    return -EINVALIDINODE;
  }

  char *buffer = new char[parentInode.size];
  int bytesRead = read(parentInodeNumber, buffer, parentInode.size);

  if (bytesRead < 0) {
    delete[] buffer;
    return -ENOTFOUND;
  }

  int numEntries = parentInode.size / sizeof(dir_ent_t);
  dir_ent_t *ent = (dir_ent_t *) buffer;

  for (int i = 0; i < numEntries; i++) {
    dir_ent_t entry = ent[i];
    if (entry.inum != 0 && name == entry.name) {
      delete[] buffer;
      return entry.inum;
    }
  }

  delete[] buffer;
  return -ENOTFOUND;
}


  /**
   * Read an inode.
   *
   * Given an inodeNumber this function will fill in the `inode` struct with
   * the type of the entry and the size of the data, in bytes, and direct blocks.
   *
   * Success: return 0
   * Failure: return -EINVALIDINODE
   * Failure modes: invalid inodeNumber
   */
int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  // load data to inode
  super_t super;
  readSuperBlock(&super);

  //LocalFileSystem::read(int inodeNumber, void *buffer, int size)
  //int inodesPerBlock = UFS_BLOCK_SIZE / sizeof(inode_t);
  if (inodeNumber < 0 || inodeNumber >= super.num_inodes) {
        return -EINVALIDINODE;
  }

  int inodesPerBlock = UFS_BLOCK_SIZE / sizeof(inode_t);
  int blockNumber = super.inode_region_addr + (inodeNumber / inodesPerBlock);
  int offset = (inodeNumber % inodesPerBlock) * sizeof(inode_t);

  char inodeBlock[UFS_BLOCK_SIZE];
  disk->readBlock(blockNumber, inodeBlock);

  memcpy(inode, inodeBlock + offset, sizeof(inode_t));
  return 0;

}


 /**
   * Read the contents of a file or directory.
   *
   * Reads up to `size` bytes of data into the buffer from file specified by
   * inodeNumber. The routine should work for either a file or directory;
   * directories should return data in the format specified by dir_ent_t.
   *
   * Success: number of bytes read
   * Failure: -EINVALIDINODE, -EINVALIDSIZE.
   * Failure modes: invalid inodeNumber, invalid size.
   */
int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  super_t super;
  readSuperBlock(&super);

  inode_t inode;
  if (stat(inodeNumber, &inode) < 0) {
    return -EINVALIDINODE;
  }

  if (size <= 0 || size > inode.size) {
    size = inode.size;
  }

  

  int bytesRead = 0;
  int blockIndex = 0; // Start with the first block
  int posInBytes = 0;

  while (bytesRead < size && blockIndex < DIRECT_PTRS) {
    // if (inode.direct[blockIndex] == 0) {
    //   break;
    // }

    char blockData[UFS_BLOCK_SIZE];
    disk->readBlock(inode.direct[blockIndex], blockData);

    int offset = posInBytes % UFS_BLOCK_SIZE;
    int bytesToCopy = std::min(UFS_BLOCK_SIZE - offset, size - bytesRead);

    memcpy((char *)buffer + bytesRead, blockData + offset, bytesToCopy);

    bytesRead += bytesToCopy;
    posInBytes += bytesToCopy;
    blockIndex++;
  }

  return bytesRead;
}

/**
   * Makes a file or directory.
   *
   * Makes a file (type == UFS_REGULAR_FILE) or directory (type == UFS_DIRECTORY)
   * in the parent directory specified by parentInodeNumber of name name.
   * 
   * For create calls, you'll need to allocate both an inode and a disk block for
   * directories. If you have allocated one of these entities but can't allocate
   * the other, make sure you free allocated inodes or disk blocks before returning 
   * an error from the call.
   *
   * Success: return the inode number of the new file or directory
   * Failure: -EINVALIDINODE, -EINVALIDNAME, -EINVALIDTYPE, -ENOTENOUGHSPACE.
   * Failure modes: parentInodeNumber does not exist or is not a directory, or
   * name is too long. If name already exists and is of the correct type,
   * return success, but if the name already exists and is of the wrong type,
   * return an error.
   */
int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  super_t super;
  readSuperBlock(&super);

  inode_t parentInode;
  if (stat(parentInodeNumber, &parentInode) < 0) {
    return -EINVALIDINODE;
  }

  if (parentInode.type != UFS_DIRECTORY) {
    return -EINVALIDTYPE;
  }

  if (name.empty() || name.length() > DIR_ENT_NAME_SIZE) {
    return -EINVALIDNAME;
  }

  char parentDirBlock[UFS_BLOCK_SIZE];
  disk->readBlock(parentInode.direct[0], parentDirBlock);

  dir_ent_t *parentEntries = (dir_ent_t *)parentDirBlock;

  for (int i = 0; i < parentInode.size / 32; i++) { 
    if (strcmp(parentEntries[i].name, name.c_str()) == 0) {
      inode_t existingInode;
      if (stat(parentEntries[i].inum, &existingInode) < 0) {
        return -EINVALIDINODE;
      }

      if (existingInode.type == type) {
        return parentEntries[i].inum;

      } else {
        return -EINVALIDTYPE;
      }
    }
  }

  int inodeMapSize = UFS_BLOCK_SIZE;
  unsigned char *inodeBitMap = new unsigned char[inodeMapSize];
  readInodeBitmap(&super, inodeBitMap);

  if (!(inodeBitMap[parentInodeNumber / 8] & (1 << (parentInodeNumber % 8)))) {
    delete[] inodeBitMap;
    return -EINVALIDINODE;
  }

  // check available inode
  int newInodeNumber = -1;
  for (int i = 0; i < super.num_inodes; i++) {
    if (!(inodeBitMap[i / 8] & (1 << (i % 8)))) {
      newInodeNumber = i;
      // inodeBitMap[i / 8] |= (1 << (i % 8));
      break;
    }
  }
  
  int dataMapSize = UFS_BLOCK_SIZE;
  unsigned char *dataBitMap = new unsigned char[dataMapSize];
  readDataBitmap(&super, dataBitMap);

  int newDataBlockNumber = -1;
  for (int i = 0; i < super.num_data; i++) {
    if (!(dataBitMap[i / 8] & (1 << (i % 8)))) {
      newDataBlockNumber = i;
      // dataBitMap[i / 8] |= (1 << (i % 8));
      break;
    }
  }

  if (newInodeNumber == -1 || newDataBlockNumber == -1 || newInodeNumber >= super.num_inodes || newDataBlockNumber >= super.num_data) {
    delete[] inodeBitMap;
    delete[] dataBitMap;
    return -ENOTENOUGHSPACE;
  }

  // inodeBitMap[newInodeNumber / 8] |= (1 << (newInodeNumber % 8));
  // dataBitMap[newBlockNumber / 8] |= (1 << (newBlockNumber % 8));

  inode_t *inodes = new inode_t[super.num_inodes];
  readInodeRegion(&super, inodes);

  inode_t &newInode = inodes[newInodeNumber];
  memset(&newInode, 0, sizeof(inode_t));
  newInode.type = type;
  newInode.size = 0;
  newInode.direct[0] = newDataBlockNumber + super.data_region_addr;

  if (type == UFS_DIRECTORY) {
    char dirBlock[UFS_BLOCK_SIZE];
    memset(dirBlock, 0, UFS_BLOCK_SIZE);

    dir_ent_t *entries = (dir_ent_t *)dirBlock;
    strncpy(entries[0].name, ".", DIR_ENT_NAME_SIZE);
    entries[0].inum = newInodeNumber;
    strncpy(entries[1].name, "..", DIR_ENT_NAME_SIZE);
    entries[1].inum = parentInodeNumber;

    newInode.size = 2 *sizeof(dir_ent_t);
    disk->writeBlock(newInode.direct[0], dirBlock);
  }

  int entryIndex = parentInode.size / 32;
  if (entryIndex > 127) {
    delete[] inodeBitMap;
    delete[] dataBitMap;
    delete[] inodes;
    return -ENOTENOUGHSPACE;
  }
  
  strncpy(parentEntries[entryIndex].name, name.c_str(), DIR_ENT_NAME_SIZE);
  parentEntries[entryIndex].inum = newInodeNumber;

  inodeBitMap[newInodeNumber / 8] |= (1 << (newInodeNumber % 8));
  dataBitMap[newDataBlockNumber / 8] |= (1 << (newDataBlockNumber % 8));

  parentInode.size += sizeof(dir_ent_t);
  disk->writeBlock(parentInode.direct[0], parentDirBlock);
  
  inodes[parentInodeNumber] = parentInode;
  writeInodeRegion(&super, inodes);
  writeInodeBitmap(&super, inodeBitMap);
  writeDataBitmap(&super, dataBitMap);

  delete[] inodeBitMap;
  delete[] dataBitMap;
  delete[] inodes;
  return newInodeNumber;
}

/**
   * Write the contents of a file.
   *
   * Writes a buffer of size to the file, replacing any content that
   * already exists.
   *
   * Success: number of bytes written
   * Failure: -EINVALIDINODE, -EINVALIDSIZE, -EINVALIDTYPE.
   * Failure modes: invalid inodeNumber, invalid size, not a regular file
   * (because you can't write to directories).
   */
int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  super_t super;
  readSuperBlock(&super);

  inode_t inode;
  if (stat(inodeNumber, &inode) < 0) {
    return -EINVALIDINODE;
  }

  return 0;
}

/**
   * Remove a file or directory.
   *
   * Removes the file or directory name from the directory specified by
   * parentInodeNumber.
   *
   * Success: 0
   * Failure: -EINVALIDINODE, -EDIRNOTEMPTY, -EINVALIDNAME, -EUNLINKNOTALLOWED
   * Failure modes: parentInodeNumber does not exist or isn't a directory,
   * directory is NOT empty, or the name is invalid. Note that the name not
   * existing is NOT a failure by our definition. You can't unlink '.' or '..'
   */
int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  super_t super;
  readSuperBlock(&super);

  inode_t parentInode;
  if (stat(parentInodeNumber, &parentInode) < 0) {
    return -EINVALIDINODE;
  }

  return 0;
}

