/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: Patrick Celedio, Gurinder Singh
* Student IDs: 920457223, 921369355
* GitHub Name: PatrickCeledio,
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

#include "fsLow.h"
#include "mfs.h"

// Magic number for fsInit.c
#define SIG 12345

#define FREE_SPACE_START_BLOCK 1

// This will help us determine the int block in which we
// found a bit of value 1 representing free block
int intBlock = 0;

struct volumeCtrlBlock{
    long signature;           //Marker left behind that can be checked 
                             // to know if the disk is setup correctly
	int blockSize;          //The size of each block in bytes
	long blockCount;       //The number of blocks in the file system
	long numFreeBlocks;   //The number of blocks not in use
	int rootDir;		 //Block number where root starts
	int freeBlockNum;   //To store the block number where our bitmap starts
} volumeCtrlBlock; 

int getFreeBlockNum(int numOfInts, int * bitVector) {
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

void setBlocksAsAllocated(int freeBlock, int blocksAllocated, int * bitVector) {
	// Set the number of bits specified in the blocksAllocated
	// to 0 starting from freeBlock
	for (int i = freeBlock; i < (freeBlock + blocksAllocated); i++) {
		bitVector[intBlock] = bitVector[intBlock] & ~(1 << (32 - i));
	}
}

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	// printf ("Initializing File System with %ld blocks with a block size of %ld\n", 
	// numberOfBlocks, blockSize);

	/* TODO: Add any code you need to initialize your file system. */
	printf("Allocating resources for VCB pointer\n");

	// We need to allocate memory in block size, because
	// that's how we read and write data on our volume.
	struct volumeCtrlBlock * vcbPtr = malloc(blockSize);

	// We need to call LBAread to access our vcb if it exists 
	// to check if we have already initialized the volume or not
	int num = LBAread(vcbPtr, 1, 0);


	printf("Number of blocks read from the LBA: %d\n", num);


	if (vcbPtr->signature == SIG){
		printf("%d\n", SIG); 
		printf("Equal\n");
	}else{
		printf("%d\n", SIG); 
		printf("Volume not formatted\n");

		vcbPtr->signature = SIG;
		vcbPtr->blockSize = MINBLOCKSIZE;
		vcbPtr->blockCount = numberOfBlocks;
		//vcbPtr->numFreeBlocks = 
 		//vcbPtr->rootDir = 
		vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;

		// We need at least 19531 bits to represent the corresponding
		// blocks, or 2442 bytes, and since we will be using ints to 
		// divide our bitVector into chunks of 32 bits we will need 611
		// ints, because 611 * 4 = 2444 bytes or 19552 bits. Since int
		// division truncates the decimal part, we need to add one to
		// the result.
		int numOfInts = (numberOfBlocks / 32) + 1;

		int * bitVector = malloc(5 * blockSize);
		
		// Block 0 is the partition table
		// Block 1 is the VCB

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

		// The following will help us determine whether a specific
		// bit is 1 or 0: bitVector[0] & (1 << i), where i represents
		// the specific bit that you want to check - 1.

		// Set all the bits starting from bit 33 to 1 (free), so
		// we need to set all the bits in allocated 611
		// ints to 1. Starting from 2nd int, since we already
		// handled the 1st int
		for (int i = 1; i < numOfInts; i++) {
          for (int j = 31; j >= 0; j--) {
			// Set bit to 1
			bitVector[i] = bitVector[i] | (1 << j);
		  }
		}

		// Write bitVector that allows us to manage our free space
		// starting at block 1 up to block 6, since our bitVector
		// takes 5 blocks
	    int numBlocksWritten = LBAwrite(bitVector, 5, FREE_SPACE_START_BLOCK);

		printf("Number of blocks written to the LBA: %d\n", numBlocksWritten);

		vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;


		//**********Get the free block number ***********
		int freeBlock = getFreeBlockNum(numOfInts, bitVector);

		printf("Free block found at: %d\n", freeBlock);

        //**********Set the allocated blocks to 0***********
		setBlocksAsAllocated(freeBlock, 5, bitVector);

		freeBlock = getFreeBlockNum(numOfInts, bitVector);

		printf("Free block found at: %d\n", freeBlock);

		// Display the bits and their values (0 or 1) in 1st int 
		// block (32 bits)
		for (int i = 31; i >= 0; i--) {
      		if (bitVector[0] & (1 << i)) {
        		printf("The value at %dth bit is: %d\n", (32 - i), 1);
      		} else {
       			printf("The value at %dth bit is: %d\n", (32 - i), 0);
      		}
    	}

	}

	return 0;
	}

	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}