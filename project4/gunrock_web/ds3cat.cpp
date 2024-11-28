#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <bitset>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << argv[0] << ": diskImageFile inodeNumber" << endl;
        return 1;
    }

    Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
    LocalFileSystem *fileSystem = new LocalFileSystem(disk);
    int inodeNumber = stoi(argv[2]);

    if (dup2(fileno(stdout), STDOUT_FILENO) == -1) {
        cerr << "Error reading file" << endl;
        delete fileSystem;
        delete disk;
        return 1;
    }

    inode_t inode;
    if (fileSystem->stat(inodeNumber, &inode) < 0) {
        cerr << "Error reading file" << endl;
        delete fileSystem;
        delete disk;
        return 1;
    }

    if (inode.type == UFS_DIRECTORY) {
        cerr << "Error reading file" << endl;
        delete fileSystem;
        delete disk;
        return 1;
    }

    super_t super;
    fileSystem->readSuperBlock(&super);

    int bitMapSize = super.data_bitmap_len * UFS_BLOCK_SIZE;
    unsigned char *dataBitmap = new unsigned char[bitMapSize];
    fileSystem->readDataBitmap(&super, dataBitmap);

    cout << "File blocks" << endl;
    for (int i = 0; i < DIRECT_PTRS; i++) {
      int absoluteBlockNumber = inode.direct[i];

      int relativeBlockNumber = absoluteBlockNumber - super.data_region_addr;

      if (relativeBlockNumber < 0 || relativeBlockNumber > super.data_region_len) {
        continue;
      }

      int byteIndex = relativeBlockNumber / 8;
      int bitIndex = relativeBlockNumber % 8;


      if (!(dataBitmap[byteIndex] & (1 << bitIndex))) {
        continue;
      }

      cout << absoluteBlockNumber << endl;
    }
    cout << endl;


    cout << "File data" << endl;

    char *buffer = new char[inode.size];
    int bytesRead = fileSystem->read(inodeNumber, buffer, inode.size);
    if (bytesRead < 0) {
      cerr << "Error reading file" << endl;
      delete[] dataBitmap;
      delete[] buffer;
      delete fileSystem;
      delete disk;
      return 1;
    }

    write(STDOUT_FILENO, buffer, bytesRead);

    delete[] dataBitmap;
    delete[] buffer;
    delete fileSystem;
    delete disk;

    return 0;
}