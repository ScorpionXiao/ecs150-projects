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

// Use this function with std::sort for directory entries
bool compareByName(const dir_ent_t& a, const dir_ent_t& b) {
    return std::strcmp(a.name, b.name) < 0;
}

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

  size_t pos = 0;
    string delimiter = "/";
    while ((pos = directory.find(delimiter)) != string::npos) {
        string component = directory.substr(0, pos);
        directory.erase(0, pos + delimiter.length());
        if (!component.empty()) {
            int nextInode = fileSystem->lookup(currentInodeNumber, component);
            if (nextInode < 0) {
                cerr << "Directory not found" << endl;
                delete fileSystem;
                delete disk;
                return 1;
            }
            currentInodeNumber = nextInode;
        }
    }

    if (!directory.empty()) {
        int nextInode = fileSystem->lookup(currentInodeNumber, directory);
        if (nextInode < 0) {
            cerr << "Directory not found" << endl;
            delete fileSystem;
            delete disk;
            return 1;
        }
        currentInodeNumber = nextInode;
    }

  inode_t inode;
  if (fileSystem->stat(currentInodeNumber, &inode) < 0) {
    cerr << "Directory not found" << endl;
    delete fileSystem;
    delete disk;
    return 1;
  }

  if (inode.type == UFS_DIRECTORY) {
        char buffer[inode.size];
        if (fileSystem->read(currentInodeNumber, buffer, UFS_BLOCK_SIZE) < 0) {
            cerr << "Directory not found" << endl;
            delete fileSystem;
            delete disk;
            return 1;
        }

        vector<dir_ent_t> entries;
        int numEntries = inode.size / sizeof(dir_ent_t);

        for (int i = 0; i < numEntries; i++) {
            dir_ent_t *entry = (dir_ent_t *)(buffer + i * sizeof(dir_ent_t));
            //if (entry->inum != 0) {  // Valid entries
            entries.push_back(*entry);
            //}
        }

        // Sort the entries alphabetically by name
        sort(entries.begin(), entries.end(), compareByName);

        // Print the sorted entries
        for (const auto &entry : entries) {
            cout << entry.inum << "\t" << entry.name << endl;
        }
    } else if (inode.type == UFS_REGULAR_FILE) {
        // Handle files: Print the inode number and name
        cout << currentInodeNumber << "\t" << directory << endl;
    } else {
        cerr << "Invalid directory or file type" << endl;
        delete fileSystem;
        delete disk;
        return 1;
    }

    // Cleanup
    delete fileSystem;
    delete disk;

    return 0;
}
