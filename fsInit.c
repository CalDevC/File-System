/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "directory.h"
#include "fsLow.h"
#include "mfs.h"

// Magic number for fsInit.c
#define SIG 90982
#define FREE_SPACE_START_BLOCK 1

// This will help us determine the int block in which we
// found a bit of value 1 representing free block
int intBlock = 0;

// Pointer to our root directory (hash table of directory entries)
// hashTable* rootDir;

int blockSizeG;
int numOfInts;

struct volumeCtrlBlock {
  long signature;      //Marker left behind that can be checked
                       //to know if the disk is setup correctly 
  int blockSize;       //The size of each block in bytes
  long blockCount;	   //The number of blocks in the file system
  long numFreeBlocks;  //The number of blocks not in use
  int rootDir;		     //Block number where root starts
  int freeBlockNum;    //To store the block number where our bitmap starts
} volumeCtrlBlock;

int getFreeBlockNum(int numOfInts, int* bitVector) {
  //**********Get the free block number ***********
  // This will help determine the first block number that is
  // free
  int freeBlock = 0;

  //****Calculate free space block number*****
  // We can use the following formula to calculate the block
  // number => (32 * i) + (32 - j), where (32 * i) will give us 
  // the number of 32 bit blocks where we found a bit of value 1
  // and we add (31 - j) which is a offset to get the block number 
  // it represents within that 32 bit block
  for (int i = 0; i < numOfInts; i++) {
    for (int j = 31; j >= 0; j--) {
      if (bitVector[i] & (1 << j)) {
        intBlock = i;
        freeBlock = (intBlock * 32) + (31 - j);
        return freeBlock;
      }
    }
  }
}

void setBlocksAsAllocated(int freeBlock, int blocksAllocated, int* bitVector) {
  // Set the number of bits specified in the blocksAllocated
  // to 0 starting from freeBlock
  freeBlock += 1;
  
  int bitNum = freeBlock - ((intBlock * 32) + 32);

  if (bitNum < 0) {
    bitNum *= -1;
  }

  // This will give us the specific bit where
  // we found the free block in the specific
  // int block
  bitNum = 32 - bitNum;

  int index = bitNum;
  int sumOfFreeBlAndBlocksAlloc = (bitNum + blocksAllocated);

  for (; index < sumOfFreeBlAndBlocksAlloc; index++) {
    if (index > 32) {
      intBlock += 1;
      index = 1;
      sumOfFreeBlAndBlocksAlloc -= 32;
    }
    bitVector[intBlock] = bitVector[intBlock] & ~(1 << (32 - index));
  }
}

//Write all directory entries in the hashTable to the disk
void writeTableData(hashTable* table, int lbaCount, int lbaPosition, int blockSize) {
  dirEntry* arr = malloc(lbaCount * blockSize);

  int j = 0;  //j will track indcies for the array

  //iterate through the whole table to find every directory entry that is in use
  for (int i = 0; i < SIZE; i++) {
    node* entry = table->entries[i];
    if (strcmp(entry->value->filename, "") != 0) {
      arr[j] = *entry->value;
      j++;

      //add other entries that are at the same hash location
      while (entry->next != NULL) {
        entry = entry->next;
        arr[j] = *entry->value;
        j++;
      }
    }

    // Don't bother lookng through rest of table if all entries are found
    if (j == table->numEntries) {
      break;
    }
  }

  //Write to the array out to the specified block numbers
  LBAwrite(arr, lbaCount, lbaPosition);
  free(arr);
}

//Read all directory entries from a certain disk location into a new hash table
hashTable* readTableData(int lbaCount, int lbaPosition, int blockSize) {
  //Read all of the entries from disk into an array
  dirEntry* arr = malloc(lbaCount * blockSize);
  LBAread(arr, lbaCount, lbaPosition);

  //Create a new hash table to be populated
  hashTable* dirPtr = hashTableInit(((lbaCount * blockSize) / sizeof(dirEntry)),
   lbaPosition);

  //Add each directory entry from the array into the hash table
  int i = 0;
  dirEntry* currDirEntry = malloc(sizeof(dirEntry));
  currDirEntry = &arr[0];

  while (strcmp(currDirEntry->filename, "") != 0) {
    setEntry(currDirEntry->filename, currDirEntry, dirPtr);
    i++;
    currDirEntry = &arr[i];
  }

  return dirPtr;
}

//Initialize the file system
int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
  printf("Initializing File System with %ld blocks with a block size of %ld\n",
    numberOfBlocks, blockSize);

  blockSizeG = blockSize;
  // We will be dealing with free space using 32 bits at a time
  // represented by 1 int that's why we need to determine how
  // many such ints we need, so we need: 19531 / 32 = 610 + 1 = 611
  // ints, because 611 * 32 = 19552 bits which are enough to
  // represent 19531 blocks. The reason why we add 1 to the 610
  // is because 610 * 32 = 19520 bits which are not enough to
  // represent 19531 blocks
  numOfInts = (numberOfBlocks / 32) + 1;

  struct volumeCtrlBlock* vcbPtr = malloc(blockSize);

  // Reads data into VCB to check signature
  LBAread(vcbPtr, 1, 0);

  if (vcbPtr->signature == SIG) {
    //Volume was already formatted
    printf("Volume is formatted!\n");
  } else {
    //Volume was not properly formatted
    vcbPtr->signature = SIG;
    vcbPtr->blockSize = blockSize;
    vcbPtr->blockCount = numberOfBlocks;
    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;

    // Since we can only read and write data to and from LBA in
    // blocks we need to malloc memory for our bitVector in
    // block sizes as well
    int* bitVector = malloc(5 * blockSize);

    // 0 = occupied
    // 1 = free

    // Set first 6 bits to 0 and the rest of 25 bits of 1st integer to 1, 
    // because block 0 of LBA is the VCB, and 1 to 5 blocks will be taken 
    // by the bitVector itself
    int totalBits = 0;
    for (int i = 31; i >= 0; i--) {
      totalBits++;
      if (i >= 26) {
        // Set bit to 0
        bitVector[0] = bitVector[0] & ~(1 << i);
      } else {
        // Set bit to 1
        bitVector[0] = bitVector[0] | (1 << i);
      }
    }

    // Set all the bits starting from bit 33 to 1
    for (int i = 1; i < numOfInts; i++) {
      for (int j = 31; j >= 0; j--) {

        // Set bit to 1
        bitVector[i] = bitVector[i] | (1 << j);
      }
    }

    // Saves starting block of the free space and root directory in the VCB
    int numBlocksWritten = LBAwrite(bitVector, 5, FREE_SPACE_START_BLOCK);

    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;
    vcbPtr->rootDir = getFreeBlockNum(numOfInts, bitVector);

    int sizeOfEntry = sizeof(dirEntry);	//48 bytes
    int dirSize = (5 * blockSize);	//2560 bytes
    int numofEntries = dirSize / sizeOfEntry; //53 entries

    // Initialize our root directory to be a new hash table of directory entries
    hashTable * rootDir = hashTableInit(numofEntries, vcbPtr->rootDir);

    // Initializing the "." current directory and the ".." parent Directory 
    dirEntry* curDir = dirEntryInit(".", 1, FREE_SPACE_START_BLOCK + numBlocksWritten,
      numofEntries, time(0), time(0));
    setEntry(curDir->filename, curDir, rootDir);

    dirEntry* parentDir = dirEntryInit("..", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir->filename, parentDir, rootDir);

    // Writes VCB to block 0
    int writeVCB = LBAwrite(vcbPtr, 1, 0);

    //Get the number of the next free block
    int freeBlock = getFreeBlockNum(numOfInts, bitVector);

    //Set the allocated blocks to 0 and the directory entry data 
    //stored in the hash table
    setBlocksAsAllocated(freeBlock, 5, bitVector);
    writeTableData(rootDir, 5, freeBlock, blockSize);

    //Update the bitvector
    LBAwrite(bitVector, 5, 1);
    free(bitVector);
  }

  free(vcbPtr);
  return 0;
}


void exitFileSystem() {
  printf("System exiting\n");
}

// Helper functions
char** stringParser(char* stringToParse) {
  // Divide the path provided by the user into
  // several sub paths
  char ** subStrings = (char**)malloc(sizeof(char*)*(strlen(stringToParse)+1));
  char *subString;
  char *savePtr;
  char *delim = "/";

  int stringCount = 0;
  subString = strtok_r(stringToParse, delim, &savePtr);

  while (subString != NULL) {
    subStrings[stringCount] = subString;
    stringCount++;
    subString = strtok_r(NULL, delim, &savePtr);
  }

  subStrings[stringCount] = subString;

  
  return subStrings;
}

//Check is a path is a directory (1 = yes, 0 = no)
int fs_isDir(char* path) {
  //Parse path
  char * pathnameCopy = malloc(strlen(path)+1);

  strcpy(pathnameCopy, path);

  char ** pathParts = stringParser(pathnameCopy);

  //Traverse the path one component at a time starting from the root directory
  hashTable* currDir = readTableData(5, 6, blockSizeG);
 
  //Continue until we have processed each component in the path
  for (int i = 0; pathParts[i] != NULL; i++) {
    //check that the location exists and that it is a directory
    dirEntry* entry = getEntry(pathParts[i], currDir);
    if (entry == NULL) {
      return 0;
    }

    if (entry->isDir == 0) {
      printf("Error: Directory entry is not a directory\n");
      return 0;
    }

    //Move the current directory to the current component's directory
    //now that it has been verified
    currDir = readTableData(5, entry->location, blockSizeG);
  }
  
  free(pathnameCopy);
  free(pathParts);
  
  return 1;
}

//Check is a path is a file (1 = yes, 0 = no)
int fs_isFile(char* path) {
  // CHANGE this
  return !fs_isDir(path);
}


// Implementation of directory functions
int fs_mkdir(const char* pathname, mode_t mode) {
  char * pathnameCopy = malloc(strlen(pathname)+1);
  strcpy(pathnameCopy, pathname);

  char ** parsedPath = stringParser(pathnameCopy);

  char * parentPath = malloc(strlen(pathname)+1);

  int k = 0;
  for (int i = 0; parsedPath[i+1] != NULL; i++) {    
    for (int j = 0; j < strlen(parsedPath[i]); j++) {
      parentPath[k] = parsedPath[i][j];
      k++;
    }

    parentPath[k] = '/';
    k++;
  }

  parentPath[k] = '\0';

  if (!fs_isDir(parentPath)) {
    printf("Error: Parent path is invalid\n");
    return -1;
  }

  if (fs_isDir((char *)pathname)) {
    printf("Error: Directory with the same name already exists\n");
    return -1;
  }

  // Reads data into VCB
  struct volumeCtrlBlock* vcbPtr = malloc(blockSizeG);
  LBAread(vcbPtr, 1, 0);


  //Continue until we have processed each component in the path
  hashTable * currDir = readTableData(5, vcbPtr->rootDir, blockSizeG);

  int i = 0;
  for (; parsedPath[i+1] != NULL; i++) {
    //check that the location exists and that it is a directory
    dirEntry* entry = getEntry(parsedPath[i], currDir);
    //Move the current directory to the current component's directory
    //now that it has been verified
    currDir = readTableData(5, entry->location, blockSizeG);
  }

  int sizeOfEntry = sizeof(dirEntry);	//48 bytes
  int dirSize = (5 * blockSizeG);	//2560 bytes
  int numofEntries = dirSize / sizeOfEntry; //53 entries

  // Get the bitVector in memory -- We need to know what
  // block is free so we can store our new directory
  // there
  int* bitVector = malloc(5 * blockSizeG);

  // Read the bitvector
  LBAread(bitVector, 5, 1);

  // Create a new directory entry
  char * newDirName = parsedPath[i];
  
  dirEntry* newEntry = malloc(sizeof(dirEntry));
  int freeBlock = getFreeBlockNum(numOfInts, bitVector);

  // Initialize the new directory entry
  strcpy(newEntry->filename, newDirName);
  newEntry->isDir = 1;
  newEntry->location = freeBlock;
  newEntry->fileSize = 5 * blockSizeG;
  newEntry->dateModified = time(0);
  newEntry->dateCreated = time(0);

  // Put the updated directory entry back
  // into the directory
  setEntry(newDirName, newEntry, currDir);

  // Initialize the directory entries within the new
  // directory
  int startBlock = getEntry(newDirName, currDir)->location;
  hashTable* dirEntries = hashTableInit(numofEntries, startBlock);

  // Initializing the "." current directory and the ".." parent Directory
  dirEntry *curDir = dirEntryInit(".", 1, freeBlock,
                                  numofEntries, time(0), time(0));
  setEntry(curDir->filename, curDir, dirEntries);

  dirEntry *parentDir = dirEntryInit("..", 1, freeBlock,
                                     numofEntries, time(0), time(0));
  setEntry(parentDir->filename, parentDir, dirEntries);

  // Write parent directory
  writeTableData(currDir, 5, currDir->location, blockSizeG);
  // Write new directory
  writeTableData(dirEntries, 5, dirEntries->location, blockSizeG);

  // Update the bit vector
  setBlocksAsAllocated(freeBlock, 5, bitVector);
  LBAwrite(bitVector, 5, 1);

  free(bitVector);
  free(newEntry); 
  free(vcbPtr);
  free(pathnameCopy); 
  free(parsedPath);
  free(parentPath);

  return 0;
}
