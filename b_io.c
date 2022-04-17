/**************************************************************
* Class:  CSC-415-0# Fall 2021
* Names: 
* Student IDs:
* GitHub Name:
* Group Name:
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"

// // Test
// #include "fsLow.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
	{
	/** TODO add all the information you need in the file control block **/
	char * buf;		//holds the open file buffer
	int index;		//holds the current position in the buffer
	int buflen;		//holds how many valid bytes are in the buffer
	int location;   //holds the file location(block number) in the volume, 
				    //so we can write to that location
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized


// This will help us determine the int block in which we
// found a bit of value 1 representing free block
int intBlock = 0;

int getFreeBlockNum() {
  //**********Get the free block number ***********
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

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

void setBlocksAsAllocated(int freeBlock, int blocksAllocated) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

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

  LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, 1);
}


//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].buf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].buf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags) 
	{
	b_io_fd returnFd;

	//*** TODO ***:  Modify to save or set any information needed
	//
	//
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();	// get our own file descriptor

	if (returnFd == -1) { // check for error - all used FCB's
		printf("Error: limit of number of open files exceeded\n");
		return -1;
	}	

	// Create a new file if the file specified in the
	// pathname doesn't already exist ?

	// Initialize a file control block for the file
	b_fcb fcb = fcbArray[returnFd];

	// To represent number of valid bytes in our buffer
	fcb.buflen = 0;
	// To represent the current position in the buffer
	fcb.index = 5;
	// To represent the location at which the current file starts
	fcb.location = getFreeBlockNum();

	fcbArray[returnFd] = fcb;
	
	return (returnFd);	// all set
	}


// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
		
		
	return (0); //Change this
	}



// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count) 
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS)) {
		return (-1); 					//invalid file descriptor
	}

	b_fcb fcb = fcbArray[fd];

	// Initially we malloc memory equivalent to 1 block we can malloc 
	// more memory as we need it
	fcb.buf = malloc(sizeof(char) * 512);

	// Store the content of the passed in buffer to our file's
	// buffer
	int availBlockBytes = 512 - 5;

	for (int i = 0; i < count; i++) {
		if (fcb.buflen >= availBlockBytes) {	
			setBlocksAsAllocated(fcb.location, 1);	
			int freeBlock = getFreeBlockNum();
			// We need to create a copy of freeBlock, because
			// we don't want the original freeBlock to get modified
			int numb = freeBlock;

			// The following algorithm will break the next freeBlock 
			// number into separate digits which we will store as 
			// characters and when we need to get the combined block
			// number we can rejoin those digits to get an int
			int j = 4;
			while (numb > 0) {
				int mod = numb % 10;
				fcb.buf[j] = mod + '0';
				numb = numb / 10;
				j--;
			}

			// If the block number is less than 5 digits
			// we need to store leading zeros
			while (j >= 0) {
				fcb.buf[j] = 0 + '0';
				j--;
			}

			// Since we have reached the limit of our current block we
			// need to write it to the volume
			LBAwrite(fcb.buf, 1, fcb.location);

			// We now set the location to the next free block
			// it will be useful when we need to write next
			// block to our volume
			fcb.location = freeBlock;

			// Start writing remaining text to the new buffer
			//free(fcb.buf);
			fcb.buf = malloc(sizeof(char) * 512);
			fcb.buflen = 0;
			fcb.index = 5;
		}

		fcb.buf[fcb.index] = buffer[i];
		fcb.index++;
		fcb.buflen++;
	}

	fcbArray[fd] = fcb;

	// Write the remaining block to the disk
	LBAwrite(fcb.buf, 1, fcb.location);
	setBlocksAsAllocated(fcb.location, 1);

	// // ***********************Test Start********************** //
	// // Get the numbers representing block number from the file
	// char blockNumbs[5];

	// for (int i = 0; i < 5; i++) {
	// 	blockNumbs[i] += fcb.buf[i];
	// }

	// // Display the block numbs
	// printf("The block numb is: %s\n", blockNumbs);

	// // Convert the characters representing block number to 
	// // an integer
	// const char * constBlockNumbs = blockNumbs;
	// printf("Converted block num is: %d\n", atoi(constBlockNumbs));

	// ***********************Test End********************** //
		
	// To indicate that the write function worked correctly we return
	// the number of bytes written
	return count;
	}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
	{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	// Get the 
		
	return (0);	//Change this
	}
	
// Interface to Close the file	
void b_close (b_io_fd fd) 
	{
	b_fcb fcb = fcbArray[fd];

	// If fcb.buf is NULL then it means we have already 
	// written it to the disk and released the memory allocated
	// for it
	if (fcb.buf != NULL) 
		{
		LBAwrite(fcb.buf, 1, fcb.location);
		setBlocksAsAllocated(fcb.location, 1);
		}
	}
