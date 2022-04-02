#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#define ENTRIES_PER_BLOCK 16

struct dirEntry {
  int location;//The block number where the start of the file is stored
  char filename[20];       //The name of the file (provided by creator)
  unsigned int fileSize;   //Length of file in bytes
  time_t dateModified;      //Date file was last modified
  time_t dateCreated;	  //Date file was created
} dirEntry;

int directoryEntry(uint64_t numberOfBlocks, uint64_t blockSize) {
  struct dirEntry* dirPtr = malloc(sizeof(dirEntry));

  dirPtr->fileSize = MINBLOCKSIZE / ENTRIES_PER_BLOCK;

  return 0;
}