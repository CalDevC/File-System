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

		int * bitVector = malloc(numberOfBlocks / 8); 

		// Block 0 is the partition table
		// Block 1 is the VCB


		// 0 = occupied
		// 1 = free
		bitVector = 0;

		for(int i = 0; i < numberOfBlocks / 8; i++){
			for(int j = 0; j < 7; j++){
				//bitVector[i] | (1 << (j - 1));
				bitVector[j] = 5;
				printf("bitVector[i]: %d\n", bitVector[i]);
			}
		}

	}



	printf("Allocating resources for LBAread block 0\n");


	return 0;
	}
	
	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}