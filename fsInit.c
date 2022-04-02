/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: Patrick Celedio, 
* Student IDs: 920457223,
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
#define FREE_SPACE_START_BLOCK 2

struct volumeCtrlBlock{
    long signature;   //Marker left behind that can be checked 
                        // to know if the disk is setup correctly
	int blockSize;            //The size of each block in bytes
	long blockCount;	        //The number of blocks in the file system
	long numFreeBlocks;      //The number of blocks not in use
	int rootDir;		 //Block number where root starts
	int freeBlockNum;    //To store the block number where our bitmap starts
} volumeCtrlBlock; 

int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize)
	{
	printf ("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);
	/* TODO: Add any code you need to initialize your file system. */
	printf("Allocating resources for VCB pointer\n");

	struct volumeCtrlBlock * vcbPtr = malloc(sizeof(volumeCtrlBlock));

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

		int memoryToAllocate = (numberOfBlocks / 8) + 3;
		//int * bitVector = malloc(memoryToAllocate); 
		int * bitVector = malloc(5 * blockSize); 

		// Block 0 is the partition table
		// Block 1 is the VCB

		// 0 = occupied
		// 1 = free

		// Set first 7 bits to 0 and the rest of
		// 25 bits of 1st integer to 1
		int totalBits = 0;
		// Set all bits starting from bit 33 to 1
		for (int i = 31; i >= 0; i--) {
			totalBits++;
			if (i >= 25) {
				// Set bit to 0
				bitVector[0] = bitVector[0] & ~(1 << i);

			} else {
				// Set bit to 1
				bitVector[0] = bitVector[0] | (1 << i);			
			}
		}

		for (int i = 31; i >= 0; i--) {
			if (bitVector[0] & (1 << i)) {
				printf("The value at %dth bit is: %d\n", (31-i) + 1, 1);	
			} else {
				printf("The value at %dth bit is: %d\n", (31-i) + 1, 0);
			}
		}

		for (int i = 1; i < (memoryToAllocate / 4); i++) {
          for (int j = 31; j >= 0; j--) {
			totalBits++;  
			// Set bit to 1
			bitVector[i] = bitVector[i] | (1 << j);			
		  }
		}

		// Display all the 19,552 bits
		int counter = 0;
		for (int i = 0; i < (memoryToAllocate / 4); i++) {
          for (int j = 0; j >= 31; j++) {
			counter++;  
			if (bitVector[i] & (1 << j)) {
				printf("The value at %dth bit is: %d\n", counter, 1);	
			} else {
				printf("The value at %dth bit is: %d\n", counter, 0);
			}
		  }
		}

		printf("Total bits are: %d\n", totalBits);

		printf("Allocating resources for LBAread block 0\n");


		/* Now write that to disk with LBAwrite(theFreeSpaceMap, 5, 1) 
		*  – that is write 5 blocks starting from block 1
		*/
		int numBlockWritten = LBAwrite(bitVector, 5, 2);

	 	printf("Number of blocks written to LBA!: %d\n", numBlockWritten);
		
		// ... Or mark it yourself if the VCB is a global structure.
		vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;
	}
	
	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}