#include <iostream>
#include <string>
#include <vector>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile parentInode directory" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " a.img 0 a" << endl;
    return 1;
  }

  // Parse command line arguments
  
  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  int parentInode = stoi(argv[2]);
  string directory = string(argv[3]);

  super_t super;
  fileSystem->readSuperBlock(&super);

  int inodeNumber = fileSystem->create(parentInode, 0, directory);

  if (inodeNumber >= 0 && inodeNumber <= super.num_inodes) {
    delete disk;
    delete fileSystem;
    return 0;
  } else {
    cerr << "Error creating directory" << endl;
    delete disk;
    delete fileSystem;
    return 1;
  }

  return 0;
}
