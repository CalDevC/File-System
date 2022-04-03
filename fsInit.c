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
#include "rootDirectory.h"
#include "fsLow.h"
#include "mfs.h"

// Magic number for fsInit.c
#define SIG 12345
#define FREE_SPACE_START_BLOCK 1

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

	struct volumeCtrlBlock * vcbPtr = malloc(blockSize);

	// Reads data into VCB to check signature
	LBAread(vcbPtr, 1, 0);

	

	if (vcbPtr->signature == SIG){
		printf("%d\n", SIG); 
		printf("Equal\n");
	}else{
		printf("%d\n", SIG); 
		printf("Volume not formatted\n");

		vcbPtr->signature = SIG;
		vcbPtr->blockSize = blockSize;
		vcbPtr->blockCount = numberOfBlocks;
		vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;

		
		// We need to add one byte so that we can allocate 2444 bytes
		int numOfInts = (numberOfBlocks / 32) + 1;
        int * bitVector = malloc(5 * blockSize);

		// 0 = occupied
		// 1 = free

		// Set first 7 bits to 0 and the rest of
		// 25 bits of 1st integer to 1
		int totalBits = 0;
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

		// Set all the bits starting from bit 33 to 1
		for (int i = 1; i < numOfInts; i++) {
          for (int j = 31; j >= 0; j--) {
			totalBits++;  
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
		vcbPtr->rootDir = FREE_SPACE_START_BLOCK + numBlocksWritten;
		
		int sizeOfEntry = sizeof(dirEntry);	//48 bytes
		int maxDirSize = (5 * blockSize);	//2560 bytes
		int numofEntries = maxDirSize - (maxDirSize % sizeOfEntry); //53 entries
		
		// Initializing the "." current directory, the ".." parent Directory 
		dirEntry* curDir = dirEntryInit(".", FREE_SPACE_START_BLOCK + numBlocksWritten,
										5 * blockSize, time(0), time(0));
		setEntry(curDir->filename, curDir, dirEntries);

		dirEntry* parentDir = dirEntryInit("..", FREE_SPACE_START_BLOCK + numBlocksWritten,
											5 * blockSize, time(0), time(0));
		setEntry(parentDir->filename, parentDir, dirEntries);

		printMap(dirEntries);
		// Writes VCB to block 0
		int writeVCB = LBAwrite(bitVector, 1, 0);
	}
		
	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}