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
// #include "directory.c"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb {
  /** TODO add all the information you need in the file control block **/
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

  hashTable* directory; 	//points to the directory that contains
                          //the opened file

  dirEntry* entry;  		//points to the directory entry associated
                        //with opened file
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

  //***************End of Permissions*******************//

  //*******************Parse path***********************//

  // char * parentPath = malloc(strlen(filename) + 1);
  // char * childPath = malloc(strlen(filename) + 1);

  // int parenItt = 0;
  // int childItt = strlen(filename) - 1;
  // int j = 0;

  // while (filename[childItt] != '/' && childItt >= 0) {
  //   childItt--;
  // }

  // while (parenItt <= childItt) {
  //   parentPath[parenItt] = filename[parenItt];
  //   parenItt++;
  // }

  // childItt = 0;
  // j = parenItt;

  // while (j < strlen(filename)) {
  //   childPath[childItt] = filename[j];
  //   childItt++;
  //   j++;
  // }

  // if (parenItt == 0) {
  //   parentPath[parenItt] = '/';
  //   parenItt++;
  // }

  // parentPath[parenItt] = '\0';
  // childPath[childItt] = '\0';

  // printf("The parent path is: %s\n", parentPath);
  // printf("The child path is: %s\n", childPath);

  //****************End of Parse path*******************//
  
  //****************Further checks**********************//
  deconPath * pathParts = splitPath(filename);
  hashTable * parentDir = getDir(pathParts->parentPath);

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

      // TODO -- We need such function, because right now fs_isFile() only
      // checks if the argument is a directory or not, it doesn't actually
      // check if such file exists

  dirEntry *dirEntry = getEntry(pathParts->childName, parentDir);

  printf("fs_isFile() returns: %d\n", fs_isFile(filename));
  printf("fs_isDir() returns: %d\n", fs_isDir(filename));


  // If the last component doesn't exist:
  if (!(fs_isFile(filename) || fs_isDir(filename))) {
    // If the O_CREAT flag is set we can create that file
    printf("Initializing new file\n");

    if (fcb.flags[2] - '0') {
      dirEntry = dirEntryInit(pathParts->childName, 0, getFreeBlockNum(),
                               0, time(0), time(0));
      setBlocksAsAllocated(dirEntry->location, 1);
      printf("In if cond before setEntry new file\n");
      setEntry(dirEntry->filename, dirEntry, parentDir);
      // writeTableData(parentDir, parentDir->location);
      printf("In if cond after setEntry new file\n");
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
      printTable(parentDir);
    if (!dirEntry) {
      printf("The dirEntry is NULL\n");
      exit(1);
    }  
    if (fcb.flags[3] - '0') {
      printf("TRUNC flag is set\n");
      printf("The file's starting location is: %d\n", dirEntry->location);

      dirEntry->fileSize = 0;

      char * buffer = malloc(blockSize);
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

      // We want to overrite the existing file's first block with
      // empty buffer
      free(buffer);
      buffer = NULL;

      buffer = calloc(blockSize, 1);
      LBAwrite(buffer, 1, dirEntry->location);
      //setBlocksAsFree(dirEntry->location, 1);

      while (nextBlock) {
        setBlocksAsFree(nextBlock, 1);

        printf("In the while loop freeing file block: %d\n", nextBlock);

        char *buffer = malloc(blockSize);
        LBAread(buffer, 1, nextBlock);

        char blockChars[6];

        for (int j = 0; j < 5; j++) {
          blockChars[j] = buffer[j];
        }
        blockChars[5] = '\0';

        // Convert the characters representing block number to
        // an integer
        const char *constBlockNumbs = blockChars;
        nextBlock = atoi(constBlockNumbs);
      }
      
      // printf("Exiting\n");
      // exit(0);
      // setBlocksAsFree(nextBlock, 1);
      
      //char * buffer = malloc(blockSize);
      printTable(parentDir);
      
    }  

  }
  //****************End of Further checks**********************//

  // if (flags == (O_WRONLY | O_CREAT | O_TRUNC)) {
  //   printf("Create the destination file if doesn't already exist\n");
  //   // Get the next available block for our file

  //   // fcb.directory = rootDir;

  //   fcb.location = 20;
  //   // printf("Dest file starting at: %d\n", fcb.location);
  //   setBlocksAsAllocated(fcb.location, 1);
  // } else if (flags == O_RDONLY) {
  //   printf("The source file must exist\n");
  //   fcb.location = 2;
  //   // printf("Src file starting at: %d\n", fcb.location);
  //   // setBlocksAsAllocated(fcb.location, 1);
  // }


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
  fcb.fileSize = dirEntry->fileSize;

  // To represent the location at which the current file starts
  fcb.location = dirEntry->location;

  // To represent the directory that contains our file
  fcb.directory = parentDir;

  printTable(parentDir);

  // To represent the directory entry associated with our file
  fcb.entry = dirEntry;

  // printf("*************About to submit fcb***************\n");
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



// Interface to write function	
int b_write(b_io_fd fd, char* buffer, int count) {
  if (startup == 0) b_init();  //Initialize our system

  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
    printf("Invalid fd in write()\n");
    return (-1); 					//invalid file descriptor
  }

  b_fcb fcb = fcbArray[fd];

  // printf("*************Before checking for permission in write*************\n");
  // We first check if this file has write flag set or not
  if (!(fcb.flags[1] - '0')) {
    printf("ERROR: Cannot write to this file\n");
    return -1;
  }

  printf("*************After checking for permission in write*************\n");


  // printf("Index in b-write() is: %d\n", fcb.index);

  // This represents the amount of text we can store in our
  // buffer its 5 less than 512 because we reserve 5 character
  // to store characters representing next block associated with
  // the file
  fcb.buflen = blockSize - fcb.index;

  // fcb.index = 5;
  // if (fcb.buflen == 0) {
  // 	return -1;
  // }

  printf("************TOP: Index in the b_write(): %d************\n", fcb.index);
  int numBytesWritten = 0;

  for (int i = 0; i < count; i++) {
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
      fcb.fileSize++;
      numBytesWritten++;
      fcb.buflen--;
      fcb.offset++;
    }

    if (fcb.buflen < 1) {
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
      // Test
      // fcb.blocksRead++;

      fcb.index = 5;
      fcb.buflen = blockSize - fcb.index;
    }

    // printf("char in fcb.buf[i]: %c\n", fcb.buf[fcb.index]);
    // printf("char in buffer[i]: %c\n", buffer[i]);
  }


  // Write the remaining block to the disk
  if (fcb.index != 5) {
    // printf("After the loop, writing the buffer at: %d\n", fcb.location);
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
    setBlocksAsAllocated(fcb.location, 1);
  }

  printf("*************Before putting fd in write*************\n");
  fcbArray[fd] = fcb;
  printf("*************After putting fd in write*************\n");

  // To indicate that the write function worked correctly we return
  // the number of bytes written
  // printf("***************num of bytes written: %d***************\n", 
  // numBytesWritten);
  // printf("i at the end of the loop of b_write is: %d\n", i);
  // printf("The buffer contains: %s\n", fcb.buf);
  // printf("******************End of b_write()*******************\n");

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
    printf("Invalid fd in read()\n");
    return (-1); 					//invalid file descriptor
  }

  // Get the file control block associated with our current file
  b_fcb fcb = fcbArray[fd];


  // We first check if this file has read flag set or not
  if (!(fcb.flags[0] - '0')) {
    printf("ERROR: Cannot read from this file\n");
    return -1;
  }

  // printf("In b_read() the offset is: %ld\n", fcb.offset);
  // printf("In b_read() the read offset is: %ld\n", fcb.readOffset);

  printf("*********Top of read***********\n");
  // printf("Top: Offset is %ld\n", fcb.offset);

  // We should only read upto the size of the source file
  if (fcb.offset >= fcb.fileSize) {
    return 0;
  }

  if (fcb.offset == 0) {
    // Read the first block associated with the opened file
    LBAread(fcb.buf, 1, fcb.location);
  }

  // // Store the content of the passed in buffer to our file's
  // // buffer
  // printf("Index in b-read() is: %d\n", fcb.index);
  fcb.buflen = blockSize - fcb.index;
  // fcb.index = 5;

  // if (fcb.buflen == 0) {
  // 	return -1;
  // }

  int numBytesRead = 0;

  for (int i = 0; i < count; i++) {
    // This marks the EOF
    if (fcb.offset >= fcb.fileSize) {
      // printf("***************Num of bytes read: %d***************\n", 
      // numBytesRead);
      // printf("i in last iteration of loop of b_read is: %d\n", i);
      // printf("The buffer contains: %s\n", buffer);
      // printf("******************End of b_read()*******************\n");
      fcbArray[fd] = fcb;
      return numBytesRead;
    }

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
    }

    if (fcb.buflen < 1) {
      // Get the characters representing next block number from the file
      // printf("*************We need another block*************\n");
      char blockChars[6];

      for (int j = 0; j < 5; j++) {
        blockChars[j] = fcb.buf[j];
      }
      blockChars[5] = '\0';

      // Convert the characters representing block number to
      // an integer
      const char* constBlockNumbs = blockChars;
      int nextBlock = atoi(constBlockNumbs);

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
  return numBytesRead;
}

// Interface to Close the file	
void b_close(b_io_fd fd) {
  // check that fd is between 0 and (MAXFCBS-1)
  if ((fd < 0) || (fd >= MAXFCBS)) {
    printf("fd is: %d\n", fd);
    return; 					//invalid file descriptor
  }


  printf("In b_close()\n");
  b_fcb fcb = fcbArray[fd];

  // We need to write the directory entry representing
  // the open file, since we might have changed the file
  // size, dateModified, or dateCreated fields
  printf("In b_close() before setting entries\n");
  if (!fcb.fileSize) {
    printf("filesize is NULL\n");
  }
  printf("The file size is: %d\n", fcb.fileSize);
  fcb.entry->fileSize = fcb.fileSize;
  fcb.entry->dateModified = time(0);

  printTable(fcb.directory);
  printf("Writing the directory at: %d\n", fcb.directory->location);
  setEntry(fcb.entry->filename, fcb.entry, fcb.directory);

  printf("Entry's filename is: %s\n", fcb.directory->entries[49]->key);

  if (!getEntry(fcb.entry->filename,fcb.directory)) {
    printf("The entry is NULL\n");
  }

  printTable(fcb.directory);
  writeTableData(fcb.directory, fcb.directory->location);
  printTable(fcb.directory);

  // To indicate that the fcb at fd is now free to use
  fcb.buf = NULL;

  printf("In b_close() before putting fd back\n");
  fcbArray[fd] = fcb;
  printf("In b_close() after putting fd back\n");


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

}
