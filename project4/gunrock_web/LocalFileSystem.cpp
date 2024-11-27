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




for debug: gdbserver localhost:1234 ./

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

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
  for (int i = 0; i < super->data_bitmap_len; i++) {
    disk->readBlock(super->data_bitmap_addr, dataBitmap + i * UFS_BLOCK_SIZE);
  }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  int inodesPerBlock = UFS_BLOCK_SIZE / sizeof(inode_t);
  for (int i = 0; i < super->inode_region_len; i++) {
    disk->readBlock(super->inode_region_addr + i, inodes + (i * inodesPerBlock));
  }
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
}

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

  //Need to load data to parentInode
  super_t super;
  readSuperBlock(&super);

  inode_t *inodes = new inode_t[super.num_inodes];
  readInodeRegion(&super, inodes);


  inode_t parentInode = inodes[parentInodeNumber];

  if (parentInode.type != UFS_DIRECTORY) {
    delete[] inodes;
    return -EINVALIDINODE;
  }


  for (int i = 0; i < DIRECT_PTRS; i++) {
    if (parentInode.direct[i] == 0) {
      continue;
    }

    char dirBlock[UFS_BLOCK_SIZE];
    disk->readBlock(parentInode.direct[i], dirBlock);

    int numEntries = UFS_BLOCK_SIZE / sizeof(dir_ent_t);
    for (int j = 0; j < numEntries; j++) {
      dir_ent_t *entry = (dir_ent_t*)(dirBlock + j * sizeof(dir_ent_t));

      if (entry->inum != 0 && name == entry->name) {
        delete[] inodes;
        return entry->inum;
      }
    }
  }

  delete[] inodes;
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

  // need to check inodeNumber-th number in bitmap
  int bitmapSize = super.inode_bitmap_len * UFS_BLOCK_SIZE;
  unsigned char *inodeBitmap = new unsigned char[bitmapSize];
  readInodeBitmap(&super, inodeBitmap);

  int byteIndex = inodeNumber / 8;
  int bitIndex = inodeNumber % 8;

  // check if Inode is allocated
  if (!(inodeBitmap[byteIndex] & (1 << bitIndex))) {
    delete[] inodeBitmap;
    return -EINVALIDINODE;
  }

  int inodesPerBlock = UFS_BLOCK_SIZE / sizeof(inode_t);
  int blockNumber = super.inode_region_addr + (inodeNumber / inodesPerBlock);
  int offset = (inodeNumber % inodesPerBlock) * sizeof(inode_t);

  char inodeBlock[UFS_BLOCK_SIZE];
  disk->readBlock(blockNumber, inodeBlock);

  memcpy(inode, inodeBlock + offset, sizeof(inode_t));

  delete[] inodeBitmap;
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

  int bitmapSize = super.inode_bitmap_len * UFS_BLOCK_SIZE;
  unsigned char *inodeBitmap = new unsigned char[bitmapSize];
  readInodeBitmap(&super, inodeBitmap);

  int byteIndex = inodeNumber / 8;
  int bitIndex = inodeNumber % 8;

  // check if Inode is allocated
  if (!(inodeBitmap[byteIndex] & (1 << bitIndex))) {
    delete[] inodeBitmap;
    return -EINVALIDINODE;
  }

  delete[] inodeBitmap;

  int inodesPerBlock = UFS_BLOCK_SIZE / sizeof(inode_t);
  int blockNumber = super.inode_region_addr + (inodeNumber / inodesPerBlock);
  int offset = (inodeNumber % inodesPerBlock) * sizeof(inode_t);

  char inodeBlock[UFS_BLOCK_SIZE];
  disk->readBlock(blockNumber, inodeBlock);

  inode_t inode;
  memcpy(&inode, inodeBlock + offset, sizeof(inode_t));

  int bytesRead = 0;
  char *outputBuffer = (char *)buffer;

  for (int i = 0; i < DIRECT_PTRS && bytesRead < size; i++) {
    if (inode.direct[i] == 0) {
      continue;
    }

    char dataBlock[UFS_BLOCK_SIZE];
    disk->readBlock(inode.direct[i], dataBlock);

    int bytesToRead = std::min(UFS_BLOCK_SIZE, size - bytesRead);

    memcpy(outputBuffer + bytesRead, dataBlock, bytesToRead);
    bytesRead += bytesToRead;
  }

  return bytesRead;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
  return 0;
}

