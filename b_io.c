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
	off_t offset;    //holds the current position in file
	off_t readOffset;
	off_t writeOffset;
	int fileSize;
	// int blocksRead;
	// int blocksWrote;
	// int startBlock;
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

	// Initially we malloc memory equivalent to 1 block we can malloc 
	// more memory as we need it
	fcb.buf = malloc(sizeof(char) * blockSize);

	// To represent number of valid bytes in our buffer
	fcb.buflen = 0;

	// To represent the current position in the buffer, the first
	// five characters are reserved [0-4] to store next free block
	// number, that's why we start from 5
	fcb.index = 5;

	// To represent the current position in the file
	fcb.offset = 0;

	fcb.readOffset = 0;

	fcb.writeOffset = 0;

	fcb.fileSize = 0;

	// Test
	// fcb.blocksRead = 0;

	// fcb.blocksWrote = 0;

	// To represent the location at which the current file starts
	fcb.location = getFreeBlockNum();
	setBlocksAsAllocated(fcb.location, 1);

	// fcb.startBlock = fcb.location;

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
	
	// If whence is SEEK_SET, we need to need to set the file's
	// index to the offset provided
	if (whence == SEEK_SET) {
		fcbArray[fd].offset = offset;
	} 
	// If whence is SEEK_CUR, we need to add offset to the file's
	// current position (index)
	else if (whence == SEEK_CUR) {
		fcbArray[fd].offset += offset;
	} 
	// If whence is SEEK_END, we need to set the file's index to
	// the size of the file plus offset

	// In order to keep track of file size we need to store and update
	// that information, so we need access to the corresponding directory
	// entry
	else if (whence == SEEK_END) {

	} 
	// We return -1 indicating that the value passed for whence is
	// not valid
	else {
		return -1;
	}

	// Upon success return the new offset position starting from the
	// beginning of the file
	return fcbArray[fd].offset;
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

	int diff = 0;
	int newBlockNum = 1;
	int oldBlockNum = 0;


	printf("*********Top of write***********\n");
	printf("Top: Offset is %ld\n", fcb.offset);
	
	if (fcb.writeOffset == 0) {
		// We need to read from the starting block number 
		// of the file
		LBAread(fcb.buf, 1, 11);
		// printf("We haven't read anything yet\n");
	} else {
		// Calculate the block that we need to read
		newBlockNum = fcb.offset / 507;
		oldBlockNum = fcb.writeOffset / 507;

		int startAt = fcb.offset % 507;
		fcb.index = startAt + 5;

		if (newBlockNum != oldBlockNum) {
			// Calculate the byte within the block from which
			// we need to start from

			diff = newBlockNum - oldBlockNum;

			// We don't need to do anything
			if (diff > 0) {
				printf("We are in a block # greater than the previous block\n");

				// for (int i = 0; i < diff; i++) {
				// 	// First we need to get the block number of next block
				// 	// that is used for our current file
				// 	// Get the characters representing next block number from the file
				// 	char blockChars[5];

				// 	for (int j = 0; j < 5; j++) {
				// 		blockChars[j] = fcb.buf[j];
				// 	}

				// 	blockChars[5] = '\0';

				// 	// Convert the characters representing block number to
				// 	// an integer
				// 	const char *constBlockNumbs = blockChars;
				// 	int nextBlock = atoi(constBlockNumbs);

				// 	// Start reading remaining text from next buffer
				// 	//free(fcb.buf);

				// 	// If nextBlock = 0, we can assume that we have reached
				// 	// the end of the file
				// 	// if (nextBlock == 0)
				// 	// {
				// 	// 	printf("Hold up at i: %d\n", i);
				// 	// 	break;
				// 	// }

				// 	fcb.buf = malloc(sizeof(char) * blockSize);
				// 	LBAread(fcb.buf, 1, nextBlock);
				// }
			}

			// The buf needs to contain the data from previous blocks
			else {
				printf("We need to overwrite the previous blocks\n");

				// We need to start from the beginning
				fcb.buf = malloc(sizeof(char) * blockSize);
				LBAread(fcb.buf, 1, 11);
				
				// This will help us keep track of the block number
				// at which our current block is stored at
				fcb.location = 11;

				// printf("Block num is: %d\n", newBlockNum);

				for (int i = 0; i < newBlockNum; i++) {
					// We need to read from the starting block number
					// of the file

					// First we need to get the block number of next block
					// that is used for our current file
					// Get the characters representing next block number from the file
					char blockChars[5];

					for (int j = 0; j < 5; j++) {
						blockChars[j] = fcb.buf[j];
					}

					blockChars[5] = '\0';

					// Convert the characters representing block number to
					// an integer
					const char *constBlockNumbs = blockChars;
					int nextBlock = atoi(constBlockNumbs);

					// Start reading remaining text from next buffer
					//free(fcb.buf);

					// If nextBlock = 0, we can assume that we have reached
					// the end of the file
					// if (nextBlock == 0)
					// {
					// 	printf("Hold up at i: %d\n", i);
					// 	break;
					// }
					fcb.location = nextBlock;

					fcb.buf = malloc(sizeof(char) * blockSize);
					LBAread(fcb.buf, 1, nextBlock);
				}
			}
			
		}
	}

	printf("Index in b-write() is: %d\n", fcb.index);
	int availBlockBytes = 512 - fcb.index;
	fcb.buflen = availBlockBytes;
	// fcb.index = 5;
	int i = 0;

	// if (fcb.buflen == 0) {
	// 	return -1;
	// }

	for (; i < count; i++) {
		// printf("buflen is: %d\n", fcb.buflen);
		if (fcb.buflen >= 1) {
			// printf("Buflen in if of read is: %d**********\n", fcb.buflen);
			// printf("char @ index: %c\n", fcb.buf[fcb.index]);
			// printf("Index in if of read is: %d*********\n", fcb.index);
			fcb.buf[fcb.index] = buffer[i];
			fcb.index++;
			// printf("Index: %d\n", fcb.index);
			fcb.buflen--;
			fcb.offset++;
			fcb.writeOffset++;
		}

		if (fcb.buflen < 1) {

			char blockChars[5];

			for (int j = 0; j < 5; j++) {
				blockChars[j] = fcb.buf[j];
			}
			blockChars[5] = '\0';

			// Convert the characters representing block number to
			// an integer
			const char * constBlockNumbs = blockChars;
			int nextBlock = atoi(constBlockNumbs);

			// If the nextBLock is 0 we know that we haven't written
			// the current block to our volume
			if (newBlockNum >= oldBlockNum) {
				// Since we have reached the limit of our current block we
				// need to write it to the volume
				int freeBlock = getFreeBlockNum();
				// We need to create a copy of freeBlock, because
				// we don't want the original freeBlock to get modified
				int numb = freeBlock;

				// The following algorithm will break the next freeBlock
				// number into separate digits which we will store as
				// characters and when we need to get the combined block
				// number we can rejoin those digits to get an int
				int j = 4;
				while (numb > 0)
				{
					int mod = numb % 10;
					fcb.buf[j] = mod + '0';
					numb = numb / 10;
					j--;
				}

				// If the block number is less than 5 digits
				// we need to store leading zeros
				while (j >= 0)
				{
					fcb.buf[j] = 0 + '0';
					j--;
				}

				// Test
				// fcb.blocksWrote++;

				// We now set the location to the next free block
				// it will be useful when we need to write next
				// block to our volume
				LBAwrite(fcb.buf, 1, fcb.location);
				setBlocksAsAllocated(fcb.location, 1);
				fcb.location = freeBlock;
				fcb.buf = malloc(sizeof(char) * blockSize);
			} 
			
			else {
				printf("new block num is: %d\n", newBlockNum);
				newBlockNum++;
				LBAwrite(fcb.buf, 1, fcb.location);
				fcb.buf = malloc(sizeof(char) * blockSize);
				setBlocksAsAllocated(fcb.location, 1);
				fcb.location = nextBlock;
				// Read the next block
				LBAread(fcb.buf, 1, fcb.location);
			}

			// Test
			// fcb.blocksRead++;

			fcb.index = 5;
			availBlockBytes = 512 - 5;
			fcb.buflen = availBlockBytes;
		}

		// printf("char in fcb.buf[i]: %c\n", fcb.buf[fcb.index]);
		// printf("char in buffer[i]: %c\n", buffer[i]);
	}

	// We need to null terminate our buffer, so it doesn't contain
	// any trailing garbage values
	// buffer[i] = '\0';

	printf("In b_write() END of loop the offset is: %ld\n", fcb.offset);
	printf("Index at the end of b_write() is: %d\n", fcb.index);
	printf("In b_write() END of loop the write offset is: %ld\n", fcb.writeOffset);

	// Store the content of the passed in buffer to our file's
	// buffer
	// int availBlockBytes = blockSize - 5;

	// printf("Index at the start of the loop: %d\n", fcb.index);
	// int mallocNew = 0;

	// for (int i = 0; i < count; i++) {
	// 	if (fcb.buflen < availBlockBytes) {
	// 		fcb.buf[fcb.index] = buffer[i];
	// 		fcb.index++;
	// 		fcb.offset++;
	// 		fcb.buflen++;
	// 	}

	// 	if (fcb.buflen >= availBlockBytes) {		
	// 		int freeBlock = getFreeBlockNum();
	// 		// We need to create a copy of freeBlock, because
	// 		// we don't want the original freeBlock to get modified
	// 		int numb = freeBlock;

	// 		// The following algorithm will break the next freeBlock 
	// 		// number into separate digits which we will store as 
	// 		// characters and when we need to get the combined block
	// 		// number we can rejoin those digits to get an int
	// 		int j = 4;
	// 		while (numb > 0) {
	// 			int mod = numb % 10;
	// 			fcb.buf[j] = mod + '0';
	// 			numb = numb / 10;
	// 			j--;
	// 		}

	// 		// If the block number is less than 5 digits
	// 		// we need to store leading zeros
	// 		while (j >= 0) {
	// 			fcb.buf[j] = 0 + '0';
	// 			j--;
	// 		}

	// 		// Since we have reached the limit of our current block we
	// 		// need to write it to the volume
	// 		LBAwrite(fcb.buf, 1, fcb.location);
	// 		setBlocksAsAllocated(fcb.location, 1);

	// 		// Test
	// 		// fcb.blocksWrote++;

	// 		// We now set the location to the next free block
	// 		// it will be useful when we need to write next
	// 		// block to our volume
	// 		fcb.location = freeBlock;

	// 		// Start writing remaining text to the new buffer
	// 		//free(fcb.buf);
	// 		fcb.buf = malloc(sizeof(char) * blockSize);
	// 		fcb.buflen = 0;
	// 		fcb.index = 5;

	// 	}

	// }


	// Write the remaining block to the disk
	if (fcb.buflen >= 1) {
		LBAwrite(fcb.buf, 1, fcb.location);
		// Test
		// fcb.blocksWrote++;
		printf("Index is: %d\n", fcb.index);
		printf("Buflen is: %d\n", fcb.buflen);
		// printf("Num blocks written: %d\n", fcb.blocksWrote);
		setBlocksAsAllocated(fcb.location, 1);
	}

	fcbArray[fd] = fcb;

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
//        size chunks needed to fill the callers request. This represents the number of
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

	// Allocate memory for our buffer
	b_fcb fcb = fcbArray[fd];

	// Since we only change the read offset in the read
	// function, so if it's 0 then it means we haven't
	// read anything yet

	// printf("In b_read() the offset is: %ld\n", fcb.offset);
	// printf("In b_read() the read offset is: %ld\n", fcb.readOffset);

	printf("*********Top of read***********\n");
	printf("Top: Offset is %ld\n", fcb.offset);

	if (fcb.readOffset == 0) {
		// We need to read from the starting block number 
		// of the file
		LBAread(fcb.buf, 1, 11);
		int startAt = fcb.offset % 507;
		fcb.index = startAt + 5;
		printf("We haven't read anything yet\n");
	} else {
		// Calculate the block that we need to read
		int newBlockNum = fcb.offset / 507;
		int oldBlockNum = fcb.readOffset / 507;

		int startAt = fcb.offset % 507;
		fcb.index = startAt + 5;

		printf("New block is : %d\n", newBlockNum);
		printf("Old block is : %d\n", oldBlockNum);

		if (newBlockNum != oldBlockNum) {
			// Calculate the byte within the block from which
			// we need to start from

			int diff = newBlockNum - oldBlockNum;

			// The buf needs to contain the data from next blocks
			if (diff > 0) {
				printf("We are in a block # greater than the previous block\n");

				for (int i = 0; i < diff; i++) {
					// First we need to get the block number of next block
					// that is used for our current file
					// Get the characters representing next block number from the file
					char blockChars[5];

					for (int j = 0; j < 5; j++) {
						blockChars[j] = fcb.buf[j];
					}

					blockChars[5] = '\0';

					// Convert the characters representing block number to
					// an integer
					const char *constBlockNumbs = blockChars;
					int nextBlock = atoi(constBlockNumbs);

					// Start reading remaining text from next buffer
					//free(fcb.buf);

					// If nextBlock = 0, we can assume that we have reached
					// the end of the file
					// if (nextBlock == 0)
					// {
					// 	printf("Hold up at i: %d\n", i);
					// 	break;
					// }

					fcb.buf = malloc(sizeof(char) * blockSize);
					LBAread(fcb.buf, 1, nextBlock);
				}
			}

			// The buf needs to contain the data from previous blocks
			else {
				printf("We are in a block # smaller than the previous block\n");

				// We need to start from the beginning
				fcb.buf = malloc(sizeof(char) * blockSize);
				LBAread(fcb.buf, 1, 11);

				// printf("Block num is: %d\n", newBlockNum);

				for (int i = 0; i < newBlockNum; i++) {
					// We need to read from the starting block number
					// of the file

					// First we need to get the block number of next block
					// that is used for our current file
					// Get the characters representing next block number from the file
					char blockChars[5];

					for (int j = 0; j < 5; j++) {
						blockChars[j] = fcb.buf[j];
					}

					blockChars[5] = '\0';

					// Convert the characters representing block number to
					// an integer
					const char *constBlockNumbs = blockChars;
					int nextBlock = atoi(constBlockNumbs);

					// Start reading remaining text from next buffer
					//free(fcb.buf);

					// If nextBlock = 0, we can assume that we have reached
					// the end of the file
					// if (nextBlock == 0)
					// {
					// 	printf("Hold up at i: %d\n", i);
					// 	break;
					// }

					fcb.buf = malloc(sizeof(char) * blockSize);
					LBAread(fcb.buf, 1, nextBlock);
				}
			}
			
		}
	}

	// fcb.buf = malloc(sizeof(char) * blockSize);
	// LBAread(fcb.buf, 1, 11);

	// // Store the content of the passed in buffer to our file's
	// // buffer
	printf("Index in b-read() is: %d\n", fcb.index);
	int availBlockBytes = 512 - fcb.index;
	fcb.buflen = availBlockBytes;
	// fcb.index = 5;
	int i = 0;

	// if (fcb.buflen == 0) {
	// 	return -1;
	// }

	for (; i < count; i++) {
		// printf("buflen is: %d\n", fcb.buflen);
		if (fcb.buflen >= 1) {
			// printf("Buflen in if of read is: %d**********\n", fcb.buflen);
			// printf("char @ index: %c\n", fcb.buf[fcb.index]);
			// printf("Index in if of read is: %d*********\n", fcb.index);
			buffer[i] = fcb.buf[fcb.index];
			fcb.index++;
			printf("char is: %d\n", buffer[i]);
			fcb.buflen--;
			fcb.offset++;
			fcb.readOffset++;
		}

		if (fcb.buflen < 1) {
			// Get the characters representing next block number from the file
			printf("*************We need another block*************\n");
			char blockChars[5];

			for (int j = 0; j < 5; j++) {
				blockChars[j] = fcb.buf[j];
			}
			blockChars[5] = '\0';

			// Convert the characters representing block number to
			// an integer
			const char * constBlockNumbs = blockChars;
			int nextBlock = atoi(constBlockNumbs);

			// Start reading remaining text from next buffer
			//free(fcb.buf);

			// If nextBlock = 0, we can assume that we have reached
			// the end of the file
			if (nextBlock == 0) {
				printf("Hold up at i: %d\n", i);
				break;
			}

			fcb.buf = malloc(sizeof(char) * blockSize);
			LBAread(fcb.buf, 1, nextBlock);

			// Test
			// fcb.blocksRead++;

			fcb.index = 5;
			availBlockBytes = 512 - 5;
			fcb.buflen = availBlockBytes;
		}

		// printf("char in fcb.buf[i]: %c\n", fcb.buf[fcb.index]);
		// printf("char in buffer[i]: %c\n", buffer[i]);
	}

	// We need to null terminate our buffer, so it doesn't contain
	// any trailing garbage values
	buffer[i] = '\0';

	printf("In b_read() END of loop the offset is: %ld\n", fcb.offset);
	printf("Index at the end of b_read() is: %d\n", fcb.index);
	printf("In b_read() END of loop the read offset is: %ld\n", fcb.readOffset);

	fcbArray[fd] = fcb;

	return count;	//Change this
	}
	
// Interface to Close the file	
void b_close (b_io_fd fd) 
	{
	b_fcb fcb = fcbArray[fd];

	// We also need to write the directory entry representing
	// the open file, since we might have changed the file
	// size, dateModified, or dateCreated fields

	// If fcb.buf is NULL then it means we have already 
	// written it to the disk and released the memory allocated
	// for it -- Not sure about this logic
	// if (fcb.buf != NULL) 
	// 	{
	// 	LBAwrite(fcb.buf, 1, fcb.location);
	// 	setBlocksAsAllocated(fcb.location, 1);
	// 	}

	}
