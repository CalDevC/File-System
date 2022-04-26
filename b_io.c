/**************************************************************
<<<<<<< HEAD
* Class:  CSC-415-0# Fall 2021
* Names:
* Student IDs:
* GitHub Name:
* Group Name:
=======
* Class:  CSC-415-02 Spring 2022
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
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

<<<<<<< HEAD
// // Test
// #include "fsLow.h"
// #include "directory.c"
=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb {
  /** TODO add all the information you need in the file control block **/
<<<<<<< HEAD
  char* buf;				//holds the open file buffer
  int index;				//holds the current position in the buffer
  int buflen;				//holds how many valid bytes are in the buffer

  int location;   		//holds the file location(block number) in the volume, 
                //so we can write to that location

  off_t offset;    		//holds the current position in file
  off_t readOffset;		//holds the reading position in the file
  off_t writeOffset;		//holds the writing position in the file

  int fileSize;			//holds the size of the opened file
  char flags[5];  		//at max we can have 4 flags set

  // hashTable* directory; 	//points to the directory that contains
  //             //the opened file

  // dirEntry* entry;  		//points to the directory entry associated
                //with opened file
=======
  char* buf;				       //holds the open file buffer
  int index;				       //holds the current position in the buffer
  int buflen;				       //holds how many valid bytes are in the buffer

  int location;   		    //holds the file location(block number) in the volume, 
                          //so we can write to that location

  off_t offset;    		    //holds the current position in file

  int fileSize;			      //holds the size of the opened file
  char flags[5];  		    //at max we can have 4 flags set

  hashTable* directory; 	//points to the parent directory that contains
                          //the opened file

  dirEntry* entry;  		  //points to the directory entry associated
                          //with opened file
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized


//Method to initialize our file system
void b_init() {
  //init fcbArray to all free
  for (int i = 0; i < MAXFCBS; i++) {
    fcbArray[i].buf = NULL; //indicates a free fcbArray
  }

  startup = 1;
}

//Method to get a free FCB element
b_io_fd b_getFCB() {
  for (int i = 0; i < MAXFCBS; i++) {
    if (fcbArray[i].buf == NULL) {
      return i;		//Not thread safe (But do not worry about it for this assignment)
    }
  }
  return (-1);  //all in use
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char* filename, int flags) {
  b_io_fd returnFd;
<<<<<<< HEAD

  //*** TODO ***:  Modify to save or set any information needed
  //
  //

  if (startup == 0) b_init();  //Initialize our system

  returnFd = b_getFCB();	// get our own file descriptor

  if (returnFd == -1) { // check for error - all used FCB's
    printf("Error: limit of number of open files exceeded\n");
    return -1;
  }
=======

  //*** TODO ***:  Modify to save or set any information needed
  //
  //

  if (startup == 0) b_init();  //Initialize our system

  returnFd = b_getFCB();	// get our own file descriptor

  if (returnFd == -1) { // check for error - all used FCB's
    printf("Error: limit of number of open files exceeded\n");
    return -1;
  }

  // Initialize a file control block for the file
  b_fcb fcb = fcbArray[returnFd];

  //****************Permissions*********************//

  // 0th index represents the read flag, and 1st index
  // represents the write flag, 2nd represents the O_CREAT
  // flag, and 3rd represents the O_TRUNC flag

  // Initialy we set all flags to 0
  for (int i = 0; i < 4; i++) {
    fcb.flags[i] = 0 + '0';
  }

  fcb.flags[4] = '\0';

  // If read and write permissions are set we can assume
  // that we can both read and write to a file, and that
  // the read only and write only flags will not be set
  if (flags & O_RDWR) {
    fcb.flags[0] = 1 + '0';
    fcb.flags[1] = 1 + '0';
  }

  // If write only flag is set then it means that we cannot read
  // so we can assume that read only flag and rdwr will not be set
  else if (flags & O_WRONLY) {
    fcb.flags[1] = 1 + '0';
  }

  else {
    fcb.flags[0] = 1 + '0';
  }

  if (flags & O_CREAT) {
    fcb.flags[2] = 1 + '0';
  }

  if (flags & O_TRUNC) {
    fcb.flags[3] = 1 + '0';
  }

  //***************End of Permissions*******************//


  //****************Further checks**********************//
  deconPath* pathParts = splitPath(filename);
  hashTable* parentDir = getDir(pathParts->parentPath);

  // If path is invalid return error
  if (!parentDir) {
    printf("ERROR: The parent path is invalid\n");
    return -1;
  }

  // If last component exists:
    // If the last component of the path exists, and is a directory
    // return error
  if (fs_isDir(filename)) {
    printf("ERROR: The last component is a directory not a file\n");
    return -1;
  }

  dirEntry* dirEntry = getEntry(pathParts->childName, parentDir);

  // If the last component doesn't exist:
  if (!(fs_isFile(filename) || fs_isDir(filename))) {
    // If the O_CREAT flag is set we can create that file
    if (fcb.flags[2] - '0') {
      int freeBlock = getFreeBlockNum(1);
      // Check if the freeBlock returned is valid or not
      if (freeBlock < 0) {
        return -1;
      }

      dirEntry = dirEntryInit(pathParts->childName, 0, freeBlock,
        0, time(0), time(0));
      setBlocksAsAllocated(dirEntry->location, 1);
      setEntry(dirEntry->filename, dirEntry, parentDir);
    }
    // else return error
    else {
      printf("Error: No such file exists and the O_CREAT flag is not set\n");
      return -1;
    }
  } else {
    // If the last component of the path exists, and is a file:
      // If the O_TRUNC flag is set, set it's length to 0 (meaning
      // clear the data from the file)
    if (!dirEntry) {
      printf("The dirEntry is NULL\n");
      exit(1);
    }
    if (fcb.flags[3] - '0') {
      dirEntry->fileSize = 0;

      char* buffer = malloc(blockSize);
      if (!buffer) {
        mallocFailed();
      }
      LBAread(buffer, 1, dirEntry->location);

      char blockChars[6];

      for (int j = 0; j < 5; j++) {
        blockChars[j] = buffer[j];
      }
      blockChars[5] = '\0';

      // Convert the characters representing block number to
      // an integer
      const char* constBlockNumbs = blockChars;
      int nextBlock = atoi(constBlockNumbs);

      free(buffer);
      buffer = NULL;

      // We want to overrite the existing file's first block with
      // empty buffer
      buffer = calloc(blockSize, 1);
      LBAwrite(buffer, 1, dirEntry->location);

      free(buffer);
      buffer = NULL;

      // In the following loop we iterate through each block associated with
      // the file and set it as free, as it will allow those blocks to be
      // overwritten
      while (nextBlock) {
        setBlocksAsFree(nextBlock, 1);

        buffer = malloc(blockSize);
        if (!buffer) {
          mallocFailed();
        }

        LBAread(buffer, 1, nextBlock);

        char blockChars[6];

        for (int j = 0; j < 5; j++) {
          blockChars[j] = buffer[j];
        }
        blockChars[5] = '\0';

        // Convert the characters representing block number to
        // an integer
        const char* constBlockNumbs = blockChars;
        nextBlock = atoi(constBlockNumbs);

        free(buffer);
        buffer = NULL;
      }

    }

  }

  // Initially we malloc memory equivalent to 1 block we can malloc 
  // more memory as we need it
  fcb.buf = calloc(sizeof(char) * blockSize, 1);
  if (!fcb.buf) {
    mallocFailed();
  }

  // To represent number of valid bytes in our buffer
  fcb.buflen = 0;

  // To represent the current position in the buffer, the first
  // five characters are reserved [0-4] to store next free block
  // number, that's why we start from 5
  fcb.index = 5;

  // To represent the current position in the file
  fcb.offset = 0;

  // If it's a new file then the file size is 0, else we need
  // to get that information from it's directory entry
  fcb.fileSize = dirEntry->fileSize;

  // To represent the location at which the current file starts
  fcb.location = dirEntry->location;

  // To represent the directory that contains our file
  fcb.directory = parentDir;

  // To represent the directory entry associated with our file
  fcb.entry = dirEntry;

  fcbArray[returnFd] = fcb;

  return (returnFd);	// all set
}
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  // Initialize a file control block for the file
  b_fcb fcb = fcbArray[returnFd];

<<<<<<< HEAD
  // If the parent path is invalid return error
  printf("The file name is: %s\n", filename);

  // If last component exists:
    // If the last component of the path exists, and is a directory
    // return error

    // If the last component of the path exists, and is a file:
      // If the O_TRUNC flag is set, set it's length to 0 (meaning
      // clear the data from the file)


  // If the last component doesn't exist:
    // If the O_CREAT flag is set we can create that file
    // else return error


  printf("****************Permissions******************\n");

  // 0th index represents the read flag, and 1st index
  // represents the write flag, 2nd represents the create
  // flag, and 3rd represents the truncate flag

  // Initialy we set all flags to 0
  for (int i = 0; i < 4; i++) {
    fcb.flags[i] = 0 + '0';
  }

  fcb.flags[4] = '\0';

  // If read and write permissions are set we can assume
  // that we can both read and write to a file, and that
  // the read only and write only flags will not be set
  if (flags & O_RDWR) {
    fcb.flags[0] = 1 + '0';
    fcb.flags[1] = 1 + '0';
    printf("O_RDWR is set\n");
  }

  // If write only flag is set then it means that we cannot read
  // so we can assume that read only flag and rdwr will not be set
  else if (flags & O_WRONLY) {
    fcb.flags[1] = 1 + '0';
    printf("O_WRONLY is set\n");
  }

  else {
    fcb.flags[0] = 1 + '0';
    printf("O_RDONLY is set\n");
  }

  if (flags & O_CREAT) {
    fcb.flags[2] = 1 + '0';
    printf("O_CREAT is set\n");
  }

  if (flags & O_TRUNC) {
    fcb.flags[3] = 1 + '0';
    printf("O_TRUNC is set\n");
  }

  printf("***************End of Permissions******************\n");

  if (flags == (O_WRONLY | O_CREAT | O_TRUNC)) {
    printf("Create the destination file if doesn't already exist\n");
    // Get the next available block for our file

    // fcb.directory = rootDir;

    fcb.location = 20;
    // printf("Dest file starting at: %d\n", fcb.location);
    setBlocksAsAllocated(fcb.location, 1);
  } else if (flags == O_RDONLY) {
    printf("The source file must exist\n");
    fcb.location = 2;
    // printf("Src file starting at: %d\n", fcb.location);
    // setBlocksAsAllocated(fcb.location, 1);
  }

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

  // To represent the read position in the file
  fcb.readOffset = 0;

  // To represent the write position in the file
  fcb.writeOffset = 0;

  // If it's a new file then the file size is 0, else we need
  // to get that information from it's directory entry
  fcb.fileSize = 0;

  // To represent the location at which the current file starts
  // fcb.startBlock = fcb.location;

  // To represent the directory that contains our file
  // fcb.directory = ;

  // To represent the directory entry associated with our file
  // fcb.entry = ;

  printf("*************About to submit fcb***************\n");
  fcbArray[returnFd] = fcb;
  printf("*************Done submitting the fcb***************\n");

  return (returnFd);	// all set
}


// Interface to seek function	
int b_seek(b_io_fd fd, off_t offset, int whence) {
  if (startup == 0) b_init();  //Initialize our system

  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
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


=======
// Interface to seek function	
int b_seek(b_io_fd fd, off_t offset, int whence) {
  if (startup == 0) b_init();  //Initialize our system

  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
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
  else if (whence == SEEK_END) {
    fcbArray[fd].offset = fcbArray[fd].fileSize + offset;
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


>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

// Interface to write function	
int b_write(b_io_fd fd, char* buffer, int count) {
  if (startup == 0) b_init();  //Initialize our system

  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
    return (-1); 					//invalid file descriptor
  }

  b_fcb fcb = fcbArray[fd];

<<<<<<< HEAD
  // printf("*************Before checking for permission in write*************\n");
  // We first check if this file has write flag set or not
=======
  // We first check if this file has write flag set or not, indicating
  // write permission
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  if (!(fcb.flags[1] - '0')) {
    printf("ERROR: Cannot write to this file\n");
    return -1;
  }

<<<<<<< HEAD
  // printf("*************After checking for permission in write*************\n");

  // We use these variable to decide whether we can keep
  // writing to our current buffer or do we need to
  // to overwrite a previously written block
  int newBlockNum = 1;
  int oldBlockNum = 0;

  // printf("*********Top of write***********\n");
  // printf("Top: Offset is %ld\n", fcb.offset);

  if (fcb.writeOffset != 0) {
    // Calculate the block that we need to work with
    newBlockNum = fcb.offset / 507;
    oldBlockNum = fcb.writeOffset / 507;

    fcb.index = fcb.offset % 507;
    fcb.index += 5;

    if (newBlockNum != oldBlockNum) {
      // Calculate the byte within the block from which
      // we need to start from
      int diff = newBlockNum - oldBlockNum;

      // The buf needs to contain the data from previous blocks
      if (diff < 0) {
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
          const char* constBlockNumbs = blockChars;
          int nextBlock = atoi(constBlockNumbs);

          fcb.location = nextBlock;

          fcb.buf = malloc(sizeof(char) * blockSize);
          LBAread(fcb.buf, 1, nextBlock);
        }
      }

    }
  }

  // printf("Index in b-write() is: %d\n", fcb.index);

  // This represents the amount of text we can store in our
  // buffer its 5 less than 512 because we reserve 5 character
=======

  // This represents the amount of text we can store in our
  // buffer, it's 5 less than 512 because we reserve 5 character
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  // to store characters representing next block associated with
  // the file
  fcb.buflen = blockSize - fcb.index;

<<<<<<< HEAD
  // fcb.index = 5;
  // if (fcb.buflen == 0) {
  // 	return -1;
  // }

  // printf("************TOP: Index in the b_write(): %d************\n", fcb.index);
  int numBytesWritten = 0;

  int i = 0;


  for (; i < count; i++) {
    // printf("buflen is: %d\n", fcb.buflen);
    if (fcb.buflen >= 1) {
      // printf("Buflen in if of read is: %d**********\n", fcb.buflen);
      // printf("char @ index: %c\n", fcb.buf[fcb.index]);
      // printf("Index in if of read is: %d*********\n", fcb.index);
      fcb.buf[fcb.index] = buffer[i];
      fcb.index++;
      // printf("Index: %d\n", fcb.index);
      // NOTE: Need to think about how to handle this if we are
      // overwriting existing content
=======
  int numBytesWritten = 0;

  // Next we write the number of bytes specified in the count variable
  // from the provided buffer to our file
  for (int i = 0; i < count; i++) {
    if (fcb.buflen >= 1) {
      fcb.buf[fcb.index] = buffer[i];
      fcb.index++;
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
      fcb.fileSize++;
      numBytesWritten++;
      fcb.buflen--;
      fcb.offset++;
<<<<<<< HEAD
      fcb.writeOffset++;
    }

    if (fcb.buflen < 1) {
      if (newBlockNum >= oldBlockNum) {
        // Since we have reached the limit of our current buffer we
        // need to write it to the volume
        int freeBlock = getFreeBlockNum();

        // We need to create a copy of freeBlock, because
        // we don't want the original freeBlock to get modified
        int numb = freeBlock;

        // The following algorithm will break the next freeBlock
        // number into separate digits which we will store as
        // characters and when we need to get the block
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

        // We now set the location to the next free block it will be 
        // useful when we need to write next block to our volume

        printf("Writing the buffer at: %d\n", fcb.location);
        // printf("The buffer is: %s\n", fcb.buf);
        // printf("We are writing the buffer to our volume: %s\n", fcb.buf);
        LBAwrite(fcb.buf, 1, fcb.location);
        // printf("*******************Done Writing*******************\n");
        setBlocksAsAllocated(freeBlock, 1);
        fcb.location = freeBlock;
        // printf("Next free block is: %d\n", freeBlock);
        fcb.buf = malloc(sizeof(char) * blockSize);
      }

      // If newBlockNum is < oldBlockNum we know that we are overwriting
      // previously written blocks
      else {
        char blockChars[5];

        for (int j = 0; j < 5; j++) {
          blockChars[j] = fcb.buf[j];
        }
        blockChars[5] = '\0';

        // Convert the characters representing block number to
        // an integer
        const char* constBlockNumbs = blockChars;
        int nextBlock = atoi(constBlockNumbs);

        // printf("new block num is: %d\n", newBlockNum);
        newBlockNum++;

        // printf("We are writing the buffer to our volume in else of loop: %s\n",
        // fcb.buf);
        LBAwrite(fcb.buf, 1, fcb.location);
        // printf("*******************Done Writing*******************\n");

        fcb.buf = malloc(sizeof(char) * blockSize);

        setBlocksAsAllocated(fcb.location, 1);

        fcb.location = nextBlock;
        // Read the next block
        LBAread(fcb.buf, 1, fcb.location);
      }

      // Test
      // fcb.blocksRead++;

      fcb.index = 5;
      fcb.buflen = blockSize - fcb.index;
    }

    // printf("char in fcb.buf[i]: %c\n", fcb.buf[fcb.index]);
    // printf("char in buffer[i]: %c\n", buffer[i]);
=======
    }

    if (fcb.buflen < 1) {
      // Since we have reached the limit of our current buffer we
      // need to write it to the volume
      int freeBlock = getFreeBlockNum(1);
      // Check if the freeBlock returned is valid or not
      if (freeBlock < 0) {
        return -1;
      }

      // We need to create a copy of freeBlock, because
      // we don't want the original freeBlock to get modified
      int numb = freeBlock;

      // The following algorithm will break the next freeBlock
      // number into separate digits which we will store as
      // characters and when we need to get the block
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

      LBAwrite(fcb.buf, 1, fcb.location);

      setBlocksAsAllocated(freeBlock, 1);

      // We now set the location to the next free block it will be
      // useful when we need to write next block to our volume
      fcb.location = freeBlock;

      free(fcb.buf);
      fcb.buf = NULL;

      fcb.buf = malloc(sizeof(char) * blockSize);
      if (!fcb.buf) {
        mallocFailed();
      }

      // Since we start with a new buffer we reset the index and buflen
      fcb.index = 5;
      fcb.buflen = blockSize - fcb.index;
    }
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  }


  // Write the remaining block to the disk
  if (fcb.index != 5) {
<<<<<<< HEAD
    printf("After the loop, writing the buffer at: %d\n", fcb.location);
    // printf("Length of our buffer is: %ld\n", strlen(fcb.buf));
    // printf("Index is: %d\n", fcb.index);
    // printf("We are writing the buffer to our volume in the if cond: %s\n", fcb.buf);
    LBAwrite(fcb.buf, 1, fcb.location);
    // printf("*******************Done Writing*******************\n");
    // Test
    // fcb.blocksWrote++;
    // printf("Index is: %d\n", fcb.index);
    // printf("Buflen is: %d\n", fcb.buflen);
    // printf("Num blocks written: %d\n", fcb.blocksWrote);
=======
    // Put 0s as a placeholder for next block 
    for (int i = 0; i < 5; i++) {
      fcb.buf[i] = 0 + '0';
    }
    LBAwrite(fcb.buf, 1, fcb.location);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    setBlocksAsAllocated(fcb.location, 1);
  }

  fcbArray[fd] = fcb;

<<<<<<< HEAD
  // To indicate that the write function worked correctly we return
  // the number of bytes written
  // printf("***************num of bytes written: %d***************\n", 
  // numBytesWritten);
  // printf("i at the end of the loop of b_write is: %d\n", i);
  // printf("The buffer contains: %s\n", fcb.buf);
  // printf("******************End of b_write()*******************\n");

=======

  // To indicate that the write function worked correctly we return
  // the number of bytes written
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  return numBytesWritten;
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
int b_read(b_io_fd fd, char* buffer, int count) {

  if (startup == 0) b_init();  //Initialize our system

  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
    return (-1); 					//invalid file descriptor
  }

  // Get the file control block associated with our current file
  b_fcb fcb = fcbArray[fd];


  // We first check if this file has read flag set or not
  if (!(fcb.flags[0] - '0')) {
    printf("ERROR: Cannot read from this file\n");
    return -1;
  }

<<<<<<< HEAD
  // printf("In b_read() the offset is: %ld\n", fcb.offset);
  // printf("In b_read() the read offset is: %ld\n", fcb.readOffset);

  // printf("*********Top of read***********\n");
  // printf("Top: Offset is %ld\n", fcb.offset);

  // We should only read upto the size of the source file
  int fileLen = 4247;

  if (fcb.readOffset >= fileLen) {
    return 0;
  }

  if (fcb.readOffset == 0) {
    // We need to read from the starting block number 
    // of the file
    LBAread(fcb.buf, 1, 11);
    fcb.index = fcb.offset % 507;
    fcb.index += 5;
    // printf("We haven't read anything yet\n");
  } else {
    // Calculate the block that we need to read
    int newBlockNum = fcb.offset / 507;
    int oldBlockNum = fcb.readOffset / 507;

    int startAt = fcb.offset % 507;
    fcb.index = startAt + 5;

    // printf("New block is : %d\n", newBlockNum);
    // printf("Old block is : %d\n", oldBlockNum);

    if (newBlockNum != oldBlockNum) {
      // Calculate the byte within the block from which
      // we need to start from
      int diff = newBlockNum - oldBlockNum;

      // The buf needs to contain the data from next blocks
      if (diff > 0) {
        // printf("We are in a block # greater than the previous block\n");

        for (int i = 0; i < diff; i++) {
          // First we need to get the block number of next block
          // that is used for our current file

          // Get the characters representing next block number from the 
          // file
          char blockChars[5];

          for (int j = 0; j < 5; j++) {
            blockChars[j] = fcb.buf[j];
          }

          blockChars[5] = '\0';

          // Convert the characters representing block number to
          // an integer
          const char* constBlockNumbs = blockChars;
          int nextBlock = atoi(constBlockNumbs);

          fcb.buf = malloc(sizeof(char) * blockSize);
          LBAread(fcb.buf, 1, nextBlock);
        }
      }

      // The buf needs to contain the data from previous blocks
      else {
        // printf("We are in a block # smaller than the previous block\n");

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
          const char* constBlockNumbs = blockChars;
          int nextBlock = atoi(constBlockNumbs);

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
  // printf("Index in b-read() is: %d\n", fcb.index);
  fcb.buflen = blockSize - fcb.index;
  // fcb.index = 5;

  // if (fcb.buflen == 0) {
  // 	return -1;
  // }

  int numBytesRead = 0;

  int i = 0;

  for (; i < count; i++) {
    // This marks the EOF
    if (fcb.readOffset >= fileLen) {
      // printf("***************Num of bytes read: %d***************\n", 
      // numBytesRead);
      // printf("i in last iteration of loop of b_read is: %d\n", i);
      // printf("The buffer contains: %s\n", buffer);
      // printf("******************End of b_read()*******************\n");
=======
  // We should only read upto the size of the source file
  if (fcb.offset >= fcb.fileSize) {
    return 0;
  }

  if (fcb.offset == 0) {
    // Read the first block associated with the opened file
    LBAread(fcb.buf, 1, fcb.location);
  }

  fcb.buflen = blockSize - fcb.index;

  int numBytesRead = 0;

  for (int i = 0; i < count; i++) {
    // This marks the EOF
    if (fcb.offset >= fcb.fileSize) {
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
      fcbArray[fd] = fcb;
      return numBytesRead;
    }

<<<<<<< HEAD
    // printf("buflen is: %d\n", fcb.buflen);
    if (fcb.buflen >= 1) {
      // printf("Buflen in if of read is: %d**********\n", fcb.buflen);
      // printf("char @ index: %c\n", fcb.buf[fcb.index]);
      // printf("Index in if of read is: %d*********\n", fcb.index);
      buffer[i] = fcb.buf[fcb.index];
      fcb.index++;
      // printf("char is: %d\n", buffer[i]);
      numBytesRead++;
      fcb.buflen--;
      fcb.offset++;
      fcb.readOffset++;
=======
    if (fcb.buflen >= 1) {
      buffer[i] = fcb.buf[fcb.index];
      fcb.index++;
      numBytesRead++;
      fcb.buflen--;
      fcb.offset++;
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    }

    if (fcb.buflen < 1) {
      // Get the characters representing next block number from the file
<<<<<<< HEAD
      // printf("*************We need another block*************\n");
      char blockChars[5];
=======
      char blockChars[6];
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

      for (int j = 0; j < 5; j++) {
        blockChars[j] = fcb.buf[j];
      }
      blockChars[5] = '\0';

      // Convert the characters representing block number to
      // an integer
      const char* constBlockNumbs = blockChars;
      int nextBlock = atoi(constBlockNumbs);

<<<<<<< HEAD
      // Start reading remaining text from next buffer
      //free(fcb.buf);

      // If nextBlock = 0, we can assume that we have reached
      // the end of the file
      // if (nextBlock == 0) {
      // 	printf("Hold up at i: %d\n", i);
      // 	break;
      // }

      fcb.buf = malloc(sizeof(char) * blockSize);
      LBAread(fcb.buf, 1, nextBlock);

      fcb.index = 5;
      fcb.buflen = blockSize - fcb.index;
    }

    // printf("char in fcb.buf[i]: %c\n", fcb.buf[fcb.index]);
    // printf("char in buffer[i]: %c\n", buffer[i]);
  }

  // We need to null terminate our buffer, so it doesn't contain
  // any trailing garbage values
  // buffer[i] = '\0';

  // printf("In b_read() END of loop the offset is: %ld\n", fcb.offset);
  // printf("Index at the end of b_read() is: %d\n", fcb.index);
  // printf("In b_read() END of loop the read offset is: %ld\n", fcb.readOffset);

  fcbArray[fd] = fcb;

  // printf("***************num of bytes read: %d***************\n", 
  // numBytesRead);
  // printf("i at the end of the loop of b_read is: %d\n", i);
  // printf("The buffer contains: %s\n", buffer);
  // printf("******************End of b_read()*******************\n");
=======
      free(fcb.buf);
      fcb.buf = NULL;

      // Start reading remaining text from next buffer
      fcb.buf = malloc(sizeof(char) * blockSize);
      if (!fcb.buf) {
        mallocFailed();
      }

      // Start reading remaining text from next buffer
      LBAread(fcb.buf, 1, nextBlock);

      // Since we start with a new buffer we reset the index and buflen
      fcb.index = 5;
      fcb.buflen = blockSize - fcb.index;
    }
  }

  fcbArray[fd] = fcb;

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  return numBytesRead;
}

// Interface to Close the file	
void b_close(b_io_fd fd) {
<<<<<<< HEAD
  b_fcb fcb = fcbArray[fd];

  // We need to write the directory entry representing
  // the open file, since we might have changed the file
  // size, dateModified, or dateCreated fields


  // To indicate that the fcb at fd is now free to use
=======
  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
    return; 					//invalid file descriptor
  }

  b_fcb fcb = fcbArray[fd];

  // We need to write the directory entry representing
  // the open file, since we might have changed the file's
  // size, dateModified, or dateCreated fields
  fcb.entry->fileSize = fcb.fileSize;
  fcb.entry->dateModified = time(0);

  setEntry(fcb.entry->filename, fcb.entry, fcb.directory);

  writeTableData(fcb.directory, fcb.directory->location);

  // To indicate that the fcb at fd is now free to use
  free(fcb.buf);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  fcb.buf = NULL;

  fcbArray[fd] = fcb;

<<<<<<< HEAD

  // dirEntry *newDir = dirEntryInit(filename, 0, freeBlock,
  // 								0, time(0), time(0));

  // // Store the directory entry for our new file in the root directory
  // setEntry(newDir->filename, newDir, rootDir);

  // // Write root directory containing our new file
  // writeTableData(rootDir, rootDir->location);

  // If fcb.buf is NULL then it means we have already 
  // written it to the disk and released the memory allocated
  // for it -- Not sure about this logic
  // if (fcb.buf != NULL) 
  // 	{
  // 	LBAwrite(fcb.buf, 1, fcb.location);
  // 	setBlocksAsAllocated(fcb.location, 1);
  // 	}

=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
}
