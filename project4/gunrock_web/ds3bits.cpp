#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << argv[0] << ": diskImageFile" << endl;
        return 1;
    }

    Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
    LocalFileSystem *fileSystem = new LocalFileSystem(disk);

    super_t super;
    fileSystem->readSuperBlock(&super);

    cout << "Super" << endl;
    cout << "inode_region_addr " << super.inode_region_addr << endl;
    cout << "inode_region_len " << super.inode_region_len << endl;
    cout << "num_inodes " << super.num_inodes << endl;
    cout << "data_region_addr " << super.data_region_addr << endl;
    cout << "data_region_len " << super.data_region_len << endl;
    cout << "num_data " << super.num_data << endl;
    cout << endl;

    cout << "Inode bitmap" <<endl;

    int inodeMapSize = super.num_inodes / 8;
    unsigned char *inodeBitMap = new unsigned char[inodeMapSize];
    fileSystem->readInodeBitmap(&super, inodeBitMap);

    for (int i = 0; i < inodeMapSize; i++) {
      cout << (unsigned int) inodeBitMap[i] << " ";
    }

    cout << endl << endl;

    delete[] inodeBitMap;

    cout << "Data bitmap" <<endl;

    int dataMapSize = super.num_data / 8;
    unsigned char *dataBitMap = new unsigned char[dataMapSize];
    fileSystem->readDataBitmap(&super, dataBitMap);

    for (int i = 0; i < dataMapSize; i++) {
      cout << (unsigned int) dataBitMap[i] << " ";
    }

    cout << endl;


    delete[] dataBitMap;
    delete fileSystem;
    delete disk;

    return 0;
}
