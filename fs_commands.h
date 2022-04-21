#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#include <time.h>
#include "mfs.h"

// Magic number for fsInit.c
#define SIG 90981
#define FREE_SPACE_START_BLOCK 1
#define NUM_FREE_SPACE_BLOCKS 5
#define DIR_SIZE 5

#ifndef FS_COMMANDS_H
#define FS_COMMANDS_H

// Pointer to our root directory (hash table of directory entries)
hashTable* workingDir;
int blockSize;
int numOfInts;

void mallocFailed();
hashTable* readTableData(int lbaPosition);
void writeTableData(hashTable* table, int lbaPosition);
int isDirWithValidPath(char* path);
deconPath* splitPath(char* fullPath);
// File system helper functions
int getFreeBlockNum();
void setBlocksAsAllocated(int freeBlock, int blocksAllocated);
void setBlocksAsFree(int freeBlock, int blocksAllocated);

hashTable* getDir(char* buf);
char** stringParser(char* inputStr);

#endif