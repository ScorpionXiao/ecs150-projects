#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

#include "StringUtils.h"
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

/*
  Use this function with std::sort for directory entries
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}
*/

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << argv[0] << ": diskImageFile directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img /a/b" << endl;
    return 1;
  }
  //./ds3ls tests/disk_images/a.img /
  // #define UFS_ROOT_DIRECTORY_INODE_NUMBER (0)
  // parse command line arguments
  // for debug: gdbserver localhost:1234 ./ds3ls tests/disk_images/a.img /
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string directory = string(argv[2]);
  
  int currentInodeNumber = UFS_ROOT_DIRECTORY_INODE_NUMBER;

  inode_t inode;
  if (fileSystem->stat(currentInodeNumber, &inode) < 0) {
    cerr << "Directory not found" << endl;
    delete fileSystem;
    delete disk;
    return 1;
  }
  //inode.type = Directory
  //inode.size = 96
  //inode.direct[0] = 6

  int lookup = fileSystem->lookup(currentInodeNumber, ".");
  cout << lookup << endl;
  char buffer[inode.size];
  if (fileSystem->read(currentInodeNumber, buffer, inode.size) < 0) {
    cerr << "Directory not found" << endl;
    delete fileSystem;
    delete disk;
    return 1;
  }



  return 0;
}
