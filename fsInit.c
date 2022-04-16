/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
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
#define SIG 90981
#define FREE_SPACE_START_BLOCK 1
#define NUM_FREE_SPACE_BLOCKS 5
#define DIR_SIZE 5

// This will help us determine the int block in which we
// found a bit of value 1 representing free block
int intBlock = 0;

// Pointer to our root directory (hash table of directory entries)
hashTable* workingDir;

int blockSize;
int numOfInts;

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

void setBlocksAsAllocated(int freeBlock, int blocksAllocated, int* bitVector) {
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

//Write all directory entries in the hashTable to the disk
void writeTableData(hashTable* table, int lbaPosition) {
  int arrNumBytes = table->maxNumEntries * sizeof(dirEntry);

  tableData* data = malloc(blockSize * DIR_SIZE);

  //Directory entries
  dirEntry* arr = malloc(arrNumBytes);

  strncpy(data->dirName, table->dirName, strlen(table->dirName));

  int j = 0;  //j will track indcies for the array

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
    if (j == table->numEntries) {
      break;
    }
  }

  data->arr = malloc(arrNumBytes);
  printf("SIZE OF ARR: %d\n", table->maxNumEntries);
  memcpy(data->arr, arr, arrNumBytes);

  //Write to the array out to the specified block numbers
  int val = LBAwrite(data, DIR_SIZE, lbaPosition);

  // printf("val is: %d\n", val);
  // if (val == DIR_SIZE) {
  //   printf("Freeing arr\n");
  //   free(arr);
  //   arr = NULL;
  //   if (arr == NULL) {
  //     printf("arr is NULL\n");
  //   }
  // }

}

//Read all directory entries from a certain disk location into a new hashmap
hashTable* readTableData(int lbaPosition) {
  int arrNumBytes = ((DIR_SIZE * blockSize) / sizeof(dirEntry)) * sizeof(dirEntry);

  //Read all of the entries into an array
  tableData* data = malloc(DIR_SIZE * blockSize);
  LBAread(data, DIR_SIZE, lbaPosition);

  dirEntry* arr = malloc(arrNumBytes);
  memcpy(arr, data->arr, arrNumBytes);

  //Create a new hash table to be populated
  hashTable* dirPtr = hashTableInit(data->dirName, ((DIR_SIZE * blockSize) / sizeof(dirEntry) - 1),
    lbaPosition);

  int i = 0;
  dirEntry* currDirEntry = malloc(sizeof(dirEntry));
  currDirEntry = &arr[0];

  while (strcmp(currDirEntry->filename, "") != 0) {
    setEntry(currDirEntry->filename, currDirEntry, dirPtr);
    i++;
    currDirEntry = &arr[i];
  }

  return dirPtr;
}

//Initialize the file system
int initFileSystem(uint64_t numberOfBlocks, uint64_t definedBlockSize) {
  printf("Initializing File System with %ld blocks with a block size of %ld\n",
    numberOfBlocks, definedBlockSize);

  blockSize = definedBlockSize;
  // We will be dealing with free space using 32 bits at a time
  // represented by 1 int that's why we need to determine how
  // many such ints we need, so we need: 19531 / 32 = 610 + 1 = 611
  // ints, because 611 * 32 = 19552 bits which are enough to
  // represent 19531 blocks. The reason why we add 1 to the 610
  // is because 610 * 32 = 19520 bits which are not enough to
  // represent 19531 blocks
  numOfInts = (numberOfBlocks / 32) + 1;

  struct volumeCtrlBlock* vcbPtr = malloc(definedBlockSize);

  // Reads data into VCB to check signature
  LBAread(vcbPtr, 1, 0);

  if (vcbPtr->signature == SIG) {
    //Volume was already formatted
  } else {
    //Volume was not properly formatted
    vcbPtr->signature = SIG;
    vcbPtr->blockSize = definedBlockSize;
    vcbPtr->blockCount = numberOfBlocks;
    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;

    // Since we can only read and write data to and from LBA in
    // blocks we need to malloc memory for our bitVector in
    // block sizes as well
    int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * definedBlockSize);

    // 0 = occupied
    // 1 = free

    // Set first 6 bits to 0 and the rest of 25 bits of 1st integer to 1, 
    // because block 0 of LBA is the VCB, and 1 to NUM_FREE_SPACE_BLOCKS blocks 
    // will be taken by the bitVector itself
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

    // Saves starting block of the free space and root directory in the VCB
    int numBlocksWritten = LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

    vcbPtr->freeBlockNum = FREE_SPACE_START_BLOCK;
    vcbPtr->rootDir = getFreeBlockNum(numOfInts, bitVector);

    int sizeOfEntry = sizeof(dirEntry);	//48 bytes
    int dirSizeInBytes = (DIR_SIZE * definedBlockSize);	//2560 bytes
    int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

    // Initialize our root directory to be a new hash table of directory entries
    hashTable* rootDir = hashTableInit("/", maxNumEntries, vcbPtr->rootDir);
    workingDir = rootDir;

    // Initializing the "." current directory and the ".." parent Directory 
    dirEntry* curDir = dirEntryInit(".", 1, FREE_SPACE_START_BLOCK + numBlocksWritten,
      dirSizeInBytes, time(0), time(0));
    setEntry(curDir->filename, curDir, rootDir);

    dirEntry* parentDir = dirEntryInit("..", 1, FREE_SPACE_START_BLOCK +
      numBlocksWritten, dirSizeInBytes, time(0), time(0));
    setEntry(parentDir->filename, parentDir, rootDir);

    // Writes VCB to block 0
    int writeVCB = LBAwrite(vcbPtr, 1, 0);

    //Get the number of the next free block
    int freeBlock = getFreeBlockNum(numOfInts, bitVector);

    //Set the allocated blocks to 0 and the directory entry data 
    //stored in the hash table
    setBlocksAsAllocated(freeBlock, DIR_SIZE, bitVector);
    writeTableData(rootDir, freeBlock);


    ////////////// TEST CODE FOR OPR/CLOSE/READ DIR //////////////
    // printf("\n\n\n");

    // fdDir* myDirPtr = fs_opendir(".");

    // struct fs_diriteminfo* myInfo = fs_readdir(myDirPtr);
    // if (myInfo == NULL) {
    //   printf("END of directory\n");
    // } else {
    //   printf("Dir entry name: %s\n", myInfo->d_name);
    // }

    // myInfo = fs_readdir(myDirPtr);
    // if (myInfo == NULL) {
    //   printf("END of directory\n");
    // } else {
    //   printf("Dir entry name: %s\n", myInfo->d_name);
    // }


    // myInfo = fs_readdir(myDirPtr);
    // if (myInfo == NULL) {
    //   printf("END of directory\n");
    // } else {
    //   printf("Dir entry name: %s\n", myInfo->d_name);
    // }

    // fs_closedir(myDirPtr);
    ////////////// END TEST CODE FOR OPR/CLOSE/READ DIR //////////////

    ///////////// TEST CODE FOR SETCWD /////////////
    // fs_mkdir("/home", 0777);
    // fs_mkdir("/home/test", 0777);
    // fs_mkdir("/home/test/succes", 0777);
    // fs_mkdir("/home/test/done", 0777);
    // printf("Set current working directory:\n");
    // fs_setcwd("/home/test");

    // printTable(workingDir);
    ///////////// END TEST CODE FOR SETCWD /////////////


    free(bitVector);
    bitVector = NULL;
  }

  free(vcbPtr);
  vcbPtr = NULL;

  return 0;
}


void exitFileSystem() {
  printf("System exiting\n");
}

// Helper functions
char** stringParser(char* stringToParse) {
  // Divide the path provided by the user into
  // several sub paths
  char** subStrings = (char**)malloc(sizeof(char*) * (strlen(stringToParse) + 1));
  char* subString;
  char* savePtr;
  char* delim = "/";

  int stringCount = 0;
  subString = strtok_r(stringToParse, delim, &savePtr);

  while (subString != NULL) {
    subStrings[stringCount] = subString;
    stringCount++;
    subString = strtok_r(NULL, delim, &savePtr);
  }

  subStrings[stringCount] = subString;


  return subStrings;
}

//Check is a path is a directory (1 = yes, 0 = no)
int fs_isDir(char* path) {
  //Parse path
  char* pathnameCopy = malloc(strlen(path) + 1);
  strcpy(pathnameCopy, path);

  char** pathParts = stringParser(pathnameCopy);

  //Traverse the path one component at a time starting from the root directory
  struct volumeCtrlBlock* vcbPtr = malloc(blockSize);
  LBAread(vcbPtr, 1, 0);

  //Continue until we have processed each component in the path
  hashTable* currDir = readTableData(vcbPtr->rootDir);

  //Continue until we have processed each component in the path
  for (int i = 0; pathParts[i] != NULL; i++) {
    //check that the location exists and that it is a directory
    dirEntry* entry = getEntry(pathParts[i], currDir);
    if (entry == NULL) {
      free(vcbPtr);
      vcbPtr = NULL;
      return 0;
    }

    if (entry->isDir == 0) {
      printf("Error: Directory entry is not a directory\n");
      free(vcbPtr);
      vcbPtr = NULL;
      return 0;
    }
    //set currDir to its hash table if so

    //Move the current directory to the current component's directory
    //now that it has been verified
    currDir = readTableData(entry->location);
  }

  free(pathnameCopy);
  pathnameCopy = NULL;
  free(pathParts);
  pathParts = NULL;

  free(vcbPtr);
  vcbPtr = NULL;
  return 1;
}

// Check is a path is a file (1 = yes, 0 = no)
int fs_isFile(char* path) {
  // CHANGE this so that we check preceeding dirs but also if it is a file
  return !fs_isDir(path);
}

// Implementation of directory functions
int fs_mkdir(const char* pathname, mode_t mode) {
  char* pathnameCopy = malloc(strlen(pathname) + 1);
  strcpy(pathnameCopy, pathname);

  char** parsedPath = stringParser(pathnameCopy);

  char* parentPath = malloc(strlen(pathname) + 1);

  int k = 0;
  for (int i = 0; parsedPath[i + 1] != NULL; i++) {
    for (int j = 0; j < strlen(parsedPath[i]); j++) {
      parentPath[k] = parsedPath[i][j];
      k++;
    }

    parentPath[k] = '/';
    k++;
  }

  parentPath[k] = '\0';

  if (!fs_isDir(parentPath)) {
    printf("Error: Parent path is invalid\n");
    return -1;
  }

  if (fs_isDir((char*)pathname)) {
    printf("Error: Directory with the same name already exists\n");
    return -1;
  }

  // Reads data into VCB
  struct volumeCtrlBlock* vcbPtr = malloc(blockSize);
  LBAread(vcbPtr, 1, 0);


  //Continue until we have processed each component in the path
  hashTable* currDir = readTableData(vcbPtr->rootDir);

  int i = 0;
  for (; parsedPath[i + 1] != NULL; i++) {
    //check that the location exists and that it is a directory
    dirEntry* entry = getEntry(parsedPath[i], currDir);
    //Move the current directory to the current component's directory
    //now that it has been verified
    currDir = readTableData(entry->location);
  }

  int sizeOfEntry = sizeof(dirEntry);	//48 bytes
  int dirSizeInBytes = (DIR_SIZE * blockSize);	//2560 bytes
  int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

  // Get the bitVector in memory -- We need to know what
  // block is free so we can store our new directory
  // there
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, 1);

  // Create a new directory entry
  char* newDirName = parsedPath[i];

  dirEntry* newEntry = malloc(sizeof(dirEntry));
  int freeBlock = getFreeBlockNum(numOfInts, bitVector);

  // Initialize the new directory entry
  strcpy(newEntry->filename, newDirName);
  newEntry->isDir = 1;
  newEntry->location = freeBlock;
  newEntry->fileSize = DIR_SIZE * blockSize;
  newEntry->dateModified = time(0);
  newEntry->dateCreated = time(0);

  // Put the updated directory entry back
  // into the directory
  setEntry(newDirName, newEntry, currDir);

  // Initialize the directory entries within the new
  // directory
  int startBlock = getEntry(newDirName, currDir)->location;
  hashTable* dirEntries = hashTableInit(newDirName, maxNumEntries, startBlock);

  // Initializing the "." current directory and the ".." parent Directory
  dirEntry* curDir = dirEntryInit(".", 1, freeBlock,
    dirSizeInBytes, time(0), time(0));
  setEntry(curDir->filename, curDir, dirEntries);

  dirEntry* parentDir = dirEntryInit("..", 1, currDir->location,
    dirSizeInBytes, time(0), time(0));
  setEntry(parentDir->filename, parentDir, dirEntries);

  // Write parent directory
  writeTableData(currDir, currDir->location);
  // Write new directory
  writeTableData(dirEntries, dirEntries->location);

  // Update the bit vector
  printf("NEW FREE BLOCK: %d\n", freeBlock);
  setBlocksAsAllocated(freeBlock, DIR_SIZE, bitVector);
  printTable(currDir);


  free(bitVector);
  bitVector = NULL;
  free(newEntry);
  newEntry = NULL;
  free(vcbPtr);
  vcbPtr = NULL;
  free(pathnameCopy);
  pathnameCopy = NULL;
  free(parsedPath);
  parsedPath = NULL;
  free(parentPath);
  parentPath = NULL;

  return 0;
}

// Opens a directory stream corresponding to 'name', and returns
// a pointer to the directory stream
fdDir* fs_opendir(const char* name) {
  fdDir* fdDir = malloc(sizeof(fdDir));
  dirEntry* reqDir = getEntry((char*)name, workingDir);
  hashTable* reqDirTable = readTableData(reqDir->location);

  fdDir->dirTable = reqDirTable;
  fdDir->maxIdx = reqDirTable->maxNumEntries;
  fdDir->d_reclen = reqDirTable->numEntries;
  fdDir->directoryStartLocation = reqDir->location;
  fdDir->dirEntryPosition = reqDirTable->maxNumEntries;

  return fdDir;
}

// Closes the directory stream associated with dirp
int fs_closedir(fdDir* dirp) {
  free(dirp->dirTable);
  dirp->dirTable = NULL;
  free(dirp);
  dirp = NULL;
  return 0;
}

struct fs_diriteminfo* fs_readdir(fdDir* dirp) {
  printf("Inside ReadDir numEntries: %d\n", dirp->dirTable->numEntries);
  //Calculate the new hash table index to use and save it to the fdDir
  int dirEntIdx = getNextIdx(dirp->dirEntryPosition, dirp->dirTable);

  if (dirEntIdx == dirp->maxIdx) {
    return NULL;
  }

  printf("FOUND INDEX: %d\n", dirEntIdx);

  dirp->dirEntryPosition = dirEntIdx;

  hashTable* dirTable = dirp->dirTable;
  dirEntry* dirEnt = dirTable->entries[dirEntIdx]->value;

  //Create and populate the directory item info pointer
  struct fs_diriteminfo* dirItemInfo = malloc(sizeof(struct fs_diriteminfo));

  strcpy(dirItemInfo->d_name, dirEnt->filename);
  dirItemInfo->d_reclen = dirEnt->fileSize;
  dirItemInfo->fileType = dirEnt->isDir ? 'd' : 'f';
  printf("Inside ReadDir numEntries: %d\n", dirp->dirTable->numEntries);
  return dirItemInfo;

}

int fs_setcwd(char* buf) {
  //Parse path
  char* pathnameCopy = malloc(strlen(buf) + 1);
  strcpy(pathnameCopy, buf);

  char** pathParts = stringParser(pathnameCopy);

  //Traverse the path one component at a time starting from the root directory
  // Reads data into VCB
  struct volumeCtrlBlock* vcbPtr = malloc(blockSize);
  LBAread(vcbPtr, 1, 0);

  //Continue until we have processed each component in the path
  hashTable* currDir = readTableData(vcbPtr->rootDir);

  //Continue until we have processed each component in the path
  for (int i = 0; pathParts[i] != NULL; i++) {
    //check that the location exists and that it is a directory
    dirEntry* entry = getEntry(pathParts[i], currDir);
    if (entry == NULL) {  //Not found
      return -1;
    }

    if (entry->isDir == 0) {  //Not a directory
      printf("Error: Directory entry is not a directory\n");
      return -1;
    }

    //Move the current directory to the current component's directory
    //now that it has been verified
    currDir = readTableData(entry->location);
  }

  workingDir = currDir;

  free(vcbPtr);
  vcbPtr = NULL;

  return 0;
}

char* fs_getcwd(char* buf, size_t size) {
  char* path = malloc(size);
  path[0] = '/';

  //Check if cwd is root
  if (strcmp(workingDir->dirName, "/") == 0) {
    strcpy(buf, path);
    // free(path);
    // path = NULL;
    return buf;
  }

  char* pathElements[size];
  int i = 0;

  hashTable* currDir = workingDir;
  dirEntry* parentDirEnt = getEntry("..", currDir);
  hashTable* parentDir = readTableData(parentDirEnt->location);
  char* newPathElem = currDir->dirName;

  //Keep traversing up until root is found
  while (strcmp(newPathElem, "/") != 0) {
    //add currDir to path elements array
    pathElements[i] = newPathElem;
    i++;

    //Move up current directory to its parent
    currDir = parentDir;
    newPathElem = currDir->dirName;

    //Set the new parent directory
    parentDirEnt = getEntry("..", currDir);
    parentDir = readTableData(parentDirEnt->location);
  }

  //Build the path
  for (int j = i - 1; j >= 0; j--) {
    strcat(path, pathElements[j]);
    strcat(path, "/");
  }

  //Add NULL char
  path[size - 1] = '\0';

  //copy path to buf
  strncpy(buf, path, size);

  // free(path);
  // path = NULL;
  return buf;
}
