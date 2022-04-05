/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: PatrickCeledio, CalDevC, GurinderS120, jonathanluu0
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

//Write all directory entries in the hashTable to the disk
void writeTableData(hashTable* table, int lbaCount, int lbaPosition, int blockSize) {
  dirEntry* arr = malloc(lbaCount * blockSize);

  //j will track indcies for the array
  int j = 0;

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

    //Don't bother lookng through rest of table if all entries are found
    if (j == table->numEntries - 1) {
      break;
    }
  }

  //Group the directory entries into chunks that fit within a single block
  int dirEntNum = 0;
  const int entriesPerBlock = blockSize / sizeof(dirEntry);

  //Assign 1 block at a time starting from the first free block
  for (int j = lbaPosition; j < lbaPosition + lbaCount; j++) {

    //Create a temporary array that will hold each chunk of directory entries
    dirEntry* tempArr = malloc(blockSize);
    int k = dirEntNum;
    for (k; k < dirEntNum + entriesPerBlock; k++) {
      tempArr[k - dirEntNum] = arr[k];
    }

    dirEntNum = dirEntNum + entriesPerBlock;

    int val = LBAwrite(tempArr, 1, j);
  }
}

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
  printf("Initializing File System with %ld blocks with a block size of %ld\n",
    numberOfBlocks, blockSize);
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


    // We will be dealing with free space using 32 bits at a time
    // represented by 1 int that's why we need to determine how
    // many such ints we need, so we need: 19531 / 32 = 610 + 1 = 611 
    // ints, because 611 * 32 = 19552 bits which are enough to
    // represent 19531 blocks. The reason why we add 1 to the 610
    // is because 610 * 32 = 19520 bits which are not enough to
    // represent 19531 blocks
    int numOfInts = (numberOfBlocks / 32) + 1;

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
      if (i >= 27) {
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
    // int writeRootDirBlocks = LBAwrite(bitVector, 5, FREE_SPACE_START_BLOCK + numBlocksWritten);

    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;
    vcbPtr->rootDir = getFreeBlockNum(numOfInts, bitVector);

    int sizeOfEntry = sizeof(dirEntry);	//48 bytes
    int maxDirSize = (5 * blockSize);	//2560 bytes
    int numofEntries = maxDirSize - (maxDirSize % sizeOfEntry); //53 entries

    // Points to an array of directory entries in a free state
    hashTable* dirEntries = hashTableInit(numofEntries);

    // Initializing the "." current directory and the ".." parent Directory 
    dirEntry* curDir = dirEntryInit(".", 1, FREE_SPACE_START_BLOCK + numBlocksWritten,
      numofEntries, time(0), time(0));
    setEntry(curDir->filename, curDir, dirEntries);

    dirEntry* parentDir = dirEntryInit("..", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir->filename, parentDir, dirEntries);

    dirEntry* parentDir1 = dirEntryInit("NAMES", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir1->filename, parentDir, dirEntries);

    dirEntry* parentDir2 = dirEntryInit("SecjdoneName", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir2->filename, parentDir, dirEntries);

    dirEntry* parentDir3 = dirEntryInit("SecondfiledfdName", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir3->filename, parentDir, dirEntries);

    dirEntry* parentDir4 = dirEntryInit("SecondjkName", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir4->filename, parentDir, dirEntries);

    dirEntry* parentDir5 = dirEntryInit("Secosadame", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir5->filename, parentDir, dirEntries);

    dirEntry* parentDir6 = dirEntryInit("Secondfilame", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir6->filename, parentDir, dirEntries);

    dirEntry* parentDir7 = dirEntryInit("SecdndfileName", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir7->filename, parentDir, dirEntries);

    dirEntry* parentDir8 = dirEntryInit("Secondfsdfgme", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir8->filename, parentDir, dirEntries);

    dirEntry* parentDir9 = dirEntryInit("SecdfileName", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, numofEntries, time(0), time(0));
    setEntry(parentDir9->filename, parentDir, dirEntries);


    // Writes VCB to block 0
    int writeVCB = LBAwrite(vcbPtr, 1, 0);

    //Get the number of the next free block
    int freeBlock = getFreeBlockNum(numOfInts, bitVector);

    //Set the allocated blocks to 0 and the directory entry data 
    //stored in the hash table
    setBlocksAsAllocated(freeBlock, 5, bitVector);
    writeTableData(dirEntries, 5, freeBlock, blockSize);
  }

  return 0;
}


void exitFileSystem() {
  printf("System exiting\n");
}
