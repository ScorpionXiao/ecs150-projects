#include <iostream>
#include <string>

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <unistd.h>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 4) {
    cerr << argv[0] << ": diskImageFile src_file dst_inode" << endl;
    cerr << "For example:" << endl;
    cerr << "    $ " << argv[0] << " tests/disk_images/a.img dthread.cpp 3" << endl;
    return 1;
  }

  // Parse command line arguments

  Disk *disk = new Disk(argv[1], UFS_BLOCK_SIZE);
  LocalFileSystem *fileSystem = new LocalFileSystem(disk);
  string srcFile = string(argv[2]);
  int dstInode = stoi(argv[3]);

  int fd = open(srcFile.c_str(), O_RDONLY);
  if (fd < 0) {
    cerr << "Could not write to dst_file" << endl;
    delete fileSystem;
    delete disk;
    return 1;
  }

  struct stat fileStat;
    if (fstat(fd, &fileStat) < 0) {
      cerr << "Could not write to dst_file" << endl;
      close(fd);
      delete fileSystem;
      delete disk;
      return 1;
    }

  off_t fileSize = fileStat.st_size;

  char *buffer = new char[fileSize];
  ssize_t bytesRead = read(fd, buffer, fileSize);

  if (bytesRead < 0) {
    cerr << "Could not write to dst_file" << endl;
    delete[] buffer;
    close(fd);
    delete fileSystem;
    delete disk;
    return 1;
  }

  int bytesWritten = fileSystem->write(dstInode, buffer, bytesRead);
  if (bytesWritten < 0) {
    cerr << "Could not write to dst_file" << endl;
    delete[] buffer;
    close(fd);
    delete fileSystem;
    delete disk;
    return 1;
  }

  delete[] buffer;
  close(fd);
  delete fileSystem;
  delete disk;
  return 0;
}
