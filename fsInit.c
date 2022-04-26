/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Function to initialize our file system. This function
* will check if the drive has been formatted. If not it will format
* the drive and if so it will do some initial set up for the file system.
*
**************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#include <time.h>
#include "b_io.h"

//Initialize the file system
int initFileSystem(uint64_t numberOfBlocks, uint64_t definedBlockSize) {

  printf("Initializing File System with %ld blocks with a block size of %ld\n",
    numberOfBlocks, definedBlockSize);

  blockSize = definedBlockSize;
  // We will be dealing with free space using 32 bits at a time
  // represented by 1 int that's why we need to determine how
  // many such ints we need, so we need: 19531 / 32 = 610 + 1 = 611
  // ints, because 611 * 32 = 19552 bits which are enough to
  // represent 19531 blocks. The reason why we add 1 to the 610
  // is because 610 * 32 = 19520 bits which are not enough to
  // represent 19531 blocks
  numOfInts = (numberOfBlocks / 32) + 1;

  struct volumeCtrlBlock* vcbPtr = malloc(definedBlockSize);
  if (!vcbPtr) {
    mallocFailed();
  }

  // Reads data into VCB to check signature
  LBAread(vcbPtr, 1, 0);

  if (vcbPtr->signature == SIG) {
    //Volume was already formatted
    int sizeOfEntry = sizeof(dirEntry);	//48 bytes
    int dirSizeInBytes = (DIR_SIZE * definedBlockSize);	//2560 bytes
    int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

    // Initialize our root directory to be a new hash table of directory entries
    hashTable* rootDir = hashTableInit("/", maxNumEntries, vcbPtr->rootDir);
    workingDir = readTableData(rootDir->location);
  } else {
    //Volume was not properly formatted
    vcbPtr->signature = SIG;
    vcbPtr->blockSize = definedBlockSize;
    vcbPtr->blockCount = numberOfBlocks;
    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;

    // Since we can only read and write data to and from LBA in
    // blocks we need to malloc memory for our bitVector in
    // block sizes as well
    int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * definedBlockSize);
    if (!bitVector) {
      mallocFailed();
    }

    // 0 = occupied
    // 1 = free

    // Set first 6 bits to 0 and the rest of 25 bits of 1st integer to 1, 
    // because block 0 of LBA is the VCB, and 1 to NUM_FREE_SPACE_BLOCKS blocks 
    // will be taken by the bitVector itself
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
    int numBlocksWritten = LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;
    int freeBlock = getFreeBlockNum(DIR_SIZE);

    // Check if the freeBlock returned is valid or not
    if (freeBlock < 0) {
      return -1;
    }
    vcbPtr->rootDir = freeBlock;

    int sizeOfEntry = sizeof(dirEntry);	//48 bytes
    int dirSizeInBytes = (DIR_SIZE * definedBlockSize);	//2560 bytes
    int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

    // Initialize our root directory to be a new hash table of directory entries
    hashTable* rootDir = hashTableInit("/", maxNumEntries, vcbPtr->rootDir);

    // Initializing the "." current directory and the ".." parent Directory 
    dirEntry* curDir = dirEntryInit(".", 1, FREE_SPACE_START_BLOCK + numBlocksWritten,
      dirSizeInBytes, time(0), time(0));
    setEntry(curDir->filename, curDir, rootDir);

    dirEntry* parentDir = dirEntryInit("..", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, dirSizeInBytes, time(0), time(0));
    setEntry(parentDir->filename, parentDir, rootDir);

    // Writes VCB to block 0
    int writeVCB = LBAwrite(vcbPtr, 1, 0);

    //Set the allocated blocks to 0 and the directory entry data 
    //stored in the hash table
    setBlocksAsAllocated(vcbPtr->rootDir, DIR_SIZE);
    writeTableData(rootDir, vcbPtr->rootDir);
    workingDir = readTableData(vcbPtr->rootDir);

    free(bitVector);
    bitVector = NULL;
  }

  free(vcbPtr);
  vcbPtr = NULL;

  return 0;
}

/****************************************************
*  exitFileSystem
****************************************************/
void exitFileSystem() {
  printf("System exiting\n");
}