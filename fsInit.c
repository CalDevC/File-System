/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Patrick Celedio, Chase Alexander
* Student IDs: 920457223, 921040156
* GitHub Name: PatrickCeledio, CalDevC
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
#define SIG 90981
#define FREE_SPACE_START_BLOCK 1

// This will help us determine the int block in which we
// found a bit of value 1 representing free block
int intBlock = 0;

struct volumeCtrlBlock {
  long signature;   //Marker left behind that can be checked 
                      // to know if the disk is setup correctly
  int blockSize;            //The size of each block in bytes
  long blockCount;	        //The number of blocks in the file system
  long numFreeBlocks;      //The number of blocks not in use
  int rootDir;		 //Block number where root starts
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
  // and we add (32 - j) which is a offset to get the block number 
  // it represents within that 32 bit block
  for (int i = 0; i < numOfInts; i++) {
    for (int j = 31; j >= 0; j--) {
      if (bitVector[i] & (1 << j)) {
        intBlock = i;
        freeBlock = (intBlock * 32) + (32 - j);
        return freeBlock;
      }
    }
  }
}

void setBlocksAsAllocated(int freeBlock, int blocksAllocated, int* bitVector) {
  // Set the number of bits specified in the blocksAllocated
  // to 0 starting from freeBlock
  for (int i = freeBlock; i < (freeBlock + blocksAllocated); i++) {
    bitVector[intBlock] = bitVector[intBlock] & ~(1 << (32 - i));
  }
}

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
  printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
  /* TODO: Add any code you need to initialize your file system. */
  printf("Allocating resources for VCB pointer\n");

  struct volumeCtrlBlock* vcbPtr = malloc(blockSize);

  // Reads data into VCB to check signature
  LBAread(vcbPtr, 1, 0);



  if (vcbPtr->signature == SIG) {
    printf("%d\n", SIG);
    printf("Equal\n");
  } else {
    printf("Signature is %d\n", SIG);
    printf("Volume not formatted\n");

    vcbPtr->signature = SIG;
    vcbPtr->blockSize = blockSize;
    vcbPtr->blockCount = numberOfBlocks;
    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;


    // We need to add one byte so that we can allocate 2444 bytes
    int numOfInts = (numberOfBlocks / 32) + 1;
    int* bitVector = malloc(5 * blockSize);

    // 0 = occupied
    // 1 = free

    // Set first 6 bits to 0 and the rest of
    // 25 bits of 1st integer to 1
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

    // Points to an array of directory entries in a free state
    hashmap* dirEntries = hashmapInit();

    // Saves starting block of the free space and root directory in the VCB
    int numBlocksWritten = LBAwrite(bitVector, 5, FREE_SPACE_START_BLOCK);
    int writeRootDirBlocks = LBAwrite(bitVector, 5, FREE_SPACE_START_BLOCK + numBlocksWritten);

    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;
    vcbPtr->rootDir = getFreeBlockNum(numOfInts, bitVector);

    int sizeOfEntry = sizeof(dirEntry);	//48 bytes
    int maxDirSize = (5 * blockSize);	//2560 bytes
    int numofEntries = maxDirSize - (maxDirSize % sizeOfEntry); //53 entries

    // Initializing the "." current directory and the ".." parent Directory 
    dirEntry* curDir = dirEntryInit(".", FREE_SPACE_START_BLOCK + numBlocksWritten,
      numofEntries, time(0), time(0));
    setEntry(curDir->filename, curDir, dirEntries);

    dirEntry* parentDir = dirEntryInit("..", FREE_SPACE_START_BLOCK + numBlocksWritten,
      numofEntries, time(0), time(0));
    setEntry(parentDir->filename, parentDir, dirEntries);


    // Writes VCB to block 0
    int writeVCB = LBAwrite(bitVector, 1, 0);

    //**********Get the free block number ***********
    int freeBlock = getFreeBlockNum(numOfInts, bitVector);

    //printf("Free block found at: %d\n", freeBlock);

        //**********Set the allocated blocks to 0***********
    setBlocksAsAllocated(freeBlock, 5, bitVector);

    freeBlock = getFreeBlockNum(numOfInts, bitVector);

    //printf("Free block found at: %d\n", freeBlock);
  }

  return 0;
}


void exitFileSystem() {
  printf("System exiting\n");
}
