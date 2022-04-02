#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#define ENTRIES_PER_BLOCK 16

typedef struct {
  int location;//The block number where the start of the file is stored
  char filename[20];       //The name of the file (provided by creator)
  unsigned int fileSize;   //Length of file in bytes
  time_t dateModified;      //Date file was last modified
  time_t dateCreated;	  //Date file was created
} dirEntry;

int directoryEntry(uint64_t numberOfBlocks, uint64_t blockSize) {
  dirEntry* dirPtr = malloc(sizeof(dirEntry));

  dirPtr->fileSize = MINBLOCKSIZE / ENTRIES_PER_BLOCK;

  return 0;
}

//Hashmap functions
#define SIZE 1000

typedef struct node {
  char key[20];
  dirEntry value;
  // node* next;  Will point to the next object at that map position
} node;

typedef struct hashmap {
  node entries[SIZE];
} hashmap;

hashmap* hashmapInit() {
  hashmap* newMap = malloc(sizeof(hashmap));

  for (int i = 0; i < SIZE; i++) {
    node* temp = NULL;
    newMap->entries[i] = *temp;
  }

  return newMap;
}

int hash(const char filename[20]) {
  int value = 1;
  for (int i = 0; i < 20; i++) {
    // printf("Char value is %d making value %d\n", filename[i], value);
    value *= 2 + filename[i];
  }

  if (value < 0) {
    value *= -1;
  }

  // printf("Total is %d\n", value);
  return value % SIZE;
}

int main() {
  printf("Hash %d\n", hash("filenames"));
}