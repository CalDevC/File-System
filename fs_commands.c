<<<<<<< HEAD
#include "fs_commands.h"

void mallocFailed() {
  printf("Call to malloc memory failed, exiting program...\n");
  exit(-1);
}


//Read all directory entries from a certain disk location into a new hashmap
hashTable* readTableData(int lbaPosition) {
  int arrNumBytes = ((DIR_SIZE * blockSize) / sizeof(dirEntry)) * sizeof(dirEntry);

  //All table data
  typedef struct tableData {
    char dirName[20];
    dirEntry arr[arrNumBytes];
  } tableData;

  //Read all of the entries into an array
=======
/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: fs_commands.c
*
* Description: This file holds the implementations of our file system
* functions defined in mfs.h along with the implementation of
* additional helper functions.
*
**************************************************************/

#include "fs_commands.h"

//Read all directory entries from a certain disk location into a new hash table
hashTable* readTableData(int lbaPosition) {
  //Calculate how many directory entries we will need to have space 
  //for in the tableData struct
  int numEntries = (DIR_SIZE * blockSize) / sizeof(dirEntry);

  //Stores all table data written to disk when it is read-in
  typedef struct tableData {
    char dirName[20];
    dirEntry arr[numEntries];
  } tableData;

  //Read all of the directory entries from the disk into an instance of 
  //tableData so that it can be loaded into the new hash table
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  tableData* data = malloc(DIR_SIZE * blockSize);
  if (!data) {
    mallocFailed();
  }
<<<<<<< HEAD
=======

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  LBAread(data, DIR_SIZE, lbaPosition);

  dirEntry* arr = data->arr;

  //Create a new hash table to be populated
  hashTable* dirPtr = hashTableInit(data->dirName, ((DIR_SIZE * blockSize) / sizeof(dirEntry) - 1),
    lbaPosition);

<<<<<<< HEAD
=======
  //Loop through all entries in arr and add them to the new hash table
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  int i = 0;
  dirEntry* currDirEntry = malloc(sizeof(dirEntry));
  if (!currDirEntry) {
    mallocFailed();
  }
  currDirEntry = &arr[0];

  while (strcmp(currDirEntry->filename, "") != 0) {
    setEntry(currDirEntry->filename, currDirEntry, dirPtr);
    i++;
    currDirEntry = &arr[i];
  }

  return dirPtr;
}


<<<<<<< HEAD
//Write all directory entries in the hashTable to the disk
void writeTableData(hashTable* table, int lbaPosition) {
  int arrNumBytes = table->maxNumEntries * sizeof(dirEntry);

  //All table data
  typedef struct tableData {
    char dirName[20];
    dirEntry arr[arrNumBytes];
  } tableData;

  tableData* data = malloc(blockSize * DIR_SIZE);
=======
//Write all directory entries in the provided hash table to the disk
void writeTableData(hashTable* table, int lbaPosition) {
  int numEntries = table->maxNumEntries * sizeof(dirEntry);

  //Stores all table data written to disk when it is read-in
  typedef struct tableData {
    char dirName[20];
    dirEntry arr[numEntries];
  } tableData;

  //malloc memory for tableData which will be written to disk 
  //and for arr which will storing all of the directories
  //found in the hash table
  tableData* data = calloc(blockSize * DIR_SIZE, 1);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  if (!data) {
    mallocFailed();
  }

<<<<<<< HEAD
  //Directory entries
  dirEntry* arr = malloc(arrNumBytes);
=======
  dirEntry* arr = calloc(numEntries, 1);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  if (!arr) {
    mallocFailed();
  }

<<<<<<< HEAD
  strncpy(data->dirName, table->dirName, strlen(table->dirName));

  int j = 0;  //j will track indcies for the array

  //iterate through the whole table to find every directory entry that is in use
=======
  //Copy the hash table's name to the tableData object so that it
  //can be written to the disk
  strncpy(data->dirName, table->dirName, strlen(table->dirName));

  int j = 0;  //j will track indcies for the array of directory entries

  //Iterate through the whole table to find every directory entry that is in use
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
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

<<<<<<< HEAD
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

=======
  memcpy(data->arr, arr, numEntries);

  //Write the array out to the specified block numbers
  int val = LBAwrite(data, DIR_SIZE, lbaPosition);


  clean(table);
  table = NULL;
  free(arr);
  arr = NULL;
  free(data);
  data = NULL;
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
}


//Check if a path is a directory (1 = yes, 0 = no, -1 = error in parent path)
int isDirWithValidPath(char* path) {
  char** parsedPath = stringParser(path);
<<<<<<< HEAD
  int absPath = strcmp(parsedPath[0], "/") == 0;

  //Check if path is root, empty, or multiple '/'
  if (parsedPath[0] == NULL || (absPath && parsedPath[1] == NULL)) {
=======

  //Determin if the provide path is absolute or relative
  int absPath = strcmp(parsedPath[0], "/") == 0;

  //Check if path is root or empty
  if (parsedPath[0] == NULL || (absPath && parsedPath[1] == NULL)) {
    //If path is empty return -1 for error otherwise return 1 because it is root
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    int result = parsedPath[0] == NULL ? -1 : 1;
    free(parsedPath);
    parsedPath = NULL;
    return result;
  }

  char* parentPath = malloc(strlen(path) + 1);
  if (!parentPath) {
    mallocFailed();
  }

  hashTable* currDir;
<<<<<<< HEAD
  if (absPath) {  //Absolute path
    // Reads data into VCB
    // struct volumeCtrlBlock* vcbPtr = malloc(blockSize);
    // if (!vcbPtr) {
    //   mallocFailed();
    // }
    // LBAread(vcbPtr, 1, 0);
    // currDir = readTableData(vcbPtr->rootDir);
    currDir = getDir("/");
    // free(vcbPtr);
    // vcbPtr = NULL;
  } else {  //Relative path
=======
  if (absPath) {  //If the path is absolute, start at root
    currDir = getDir("/");
  } else {  //Otherwise start in the current working directory
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    currDir = readTableData(workingDir->location);
  }

  int i = 0;

<<<<<<< HEAD
=======
  //If the path is absolute we will skip the first component of the path
  //because we are already starting at root
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  if (absPath) {
    i++;
  }

<<<<<<< HEAD
  dirEntry* entry;

  for (; parsedPath[i + 1] != NULL; i++) {
    //check that the location exists and that it is a directory
    // printf("Getting entry\n");
    entry = getEntry(parsedPath[i], currDir);
    // printf("Entry is %s\n", entry->filename);

    if (entry == NULL || entry->isDir == 0) {
      printf("Error: Part of parent path does not exist\n");
=======
  //Iterate through each component in the path and check that the location 
  //exists and that it is a directory (validate the parent path)
  dirEntry* entry;

  //only check up to the second to last component because we are only 
  //validating the parent path not the entire path
  for (; parsedPath[i + 1] != NULL; i++) {
    entry = getEntry(parsedPath[i], currDir);

    //If the current component does not exist or is not a directory 
    //then parent path is invalid
    if (entry == NULL || entry->isDir == 0) {
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
      free(parsedPath);
      parsedPath = NULL;
      free(parentPath);
      parentPath = NULL;
      free(currDir);
      currDir = NULL;
      return -1;
    }

    //Move the current directory to the current component's directory
    //now that it has been verified
    free(currDir);
    currDir = readTableData(entry->location);
  }

  //Check that the final component in the path is a directory
  entry = getEntry(parsedPath[i], currDir);

<<<<<<< HEAD

=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  free(parsedPath);
  parsedPath = NULL;
  free(parentPath);
  parentPath = NULL;

<<<<<<< HEAD
  if (entry == NULL) {
    return 0;
  }

=======
  //Error if the last path component does not exist
  if (entry == NULL) {
    return -1;
  }

  //Now that we know the path is valid we can return either 
  //0 if it is a file or 1 if it is a directory
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  int result = entry->isDir;

  free(currDir);
  currDir = NULL;

  return result;
}


<<<<<<< HEAD
=======
//Seperate a path into its parent path and child compoent and store it 
//in a deconstructed path (deconPath) struct
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
deconPath* splitPath(char* fullPath) {
  char** parsedPath = stringParser(fullPath);
  char* parentPath = malloc(strlen(fullPath) + 1);
  if (!parentPath) {
    mallocFailed();
  }

<<<<<<< HEAD
=======
  //Build the parent path by concatenating each of the path components 
  //provided by stringParser (excluding the final one) with a '/' in 
  //between each component and a NULL character at the end
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  int k = 0;
  int i = 0;
  for (; parsedPath[i + 1] != NULL; i++) {
    for (int j = 0; j < strlen(parsedPath[i]); j++) {
      parentPath[k] = parsedPath[i][j];
      k++;
    }

<<<<<<< HEAD
    if (parentPath[k - 1] != '/' && parentPath[k - 1] != '.') {
=======
    if (parentPath[k - 1] != '/') {
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
      parentPath[k] = '/';
      k++;
    }
  }

  parentPath[k] = '\0';

<<<<<<< HEAD
=======
  //Add the components to a deconstructed path struct
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  deconPath* pathParts = malloc(sizeof(deconPath));
  if (!pathParts) {
    mallocFailed();
  }
  pathParts->parentPath = parentPath;
  pathParts->childName = parsedPath[i];

<<<<<<< HEAD
=======
  //Make sure that parentPath and childName are always initialized with 
  //something. Infer that we are using the current directory if none 
  //is specified
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  if (strcmp(pathParts->parentPath, "") == 0) {
    pathParts->parentPath = ".";
  } else if (strcmp(pathParts->childName, "") == 0) {
    pathParts->childName = pathParts->parentPath;
    pathParts->parentPath = ".";
  }

  return pathParts;
}

<<<<<<< HEAD
// This will help us determine the int block in which we
// found a bit of value 1 representing free block
int intBlock = 0;

int getFreeBlockNum() {
  //**********Get the free block number ***********
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
=======
int getFreeBlockNum(int getNumBlocks) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

  // This will help determine the first block number that is
  // free
<<<<<<< HEAD
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
=======
  int freeBlock = -1;

  // Whenever we find a free block we subtract one from the blocksToFind
  // and when it reaches 0, we know that we have found the specified
  // number of contiguous free blocks
  int blocksToFind = getNumBlocks;

  //****Calculate free space block number*****
  // We can use the following formula to calculate the block
  // number => (32 * intBlock) + (31 - j), where (32 * intBlock)
  // will give us the number of 32 bit blocks where we found a bit 
  // of value 1 and we add (31 - j) which is a offset to get the 
  // block number it represents within that 32 bit block
  for (int i = 0; i < numOfInts; i++) {
    for (int j = 31; j >= 0; j--) {
      // If the 'if condition' is true that we have found a free block
      if (bitVector[i] & (1 << j)) {
        blocksToFind--;

        // If freeBlock is -1 then it means that the first free block
        // has been found, so we calculate it's position in the bitVector
        if (freeBlock == -1) {
          intBlock = i;
          freeBlock = (intBlock * 32) + (31 - j);
        }

        // If the blocksToFind is 0 than we have found the contiguous blocks
        // that the caller asked for
        if (blocksToFind == 0) {
          return freeBlock;
        }
      }

      // If the freeBlock is not -1 and the bit is 0 then it means that we have
      // to start looking for contiguous free blocks again, since we have found a 
      // block that is not free after finding a block that was free, therefore
      // blocks are not contiguous
      else if (freeBlock != -1) {
        freeBlock = -1;
        blocksToFind = getNumBlocks;
      }
    }
  }

  printf("Error: Couldn't find %d contiguous free blocks\n", getNumBlocks);
  return -1;
}


//Updates the free space bit vector with allocated blocks
void setBlocksAsAllocated(int freeBlock, int blocksAllocated) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  freeBlock += 1;

  int bitNum = freeBlock - ((intBlock * 32) + 32);

  if (bitNum < 0) {
    bitNum *= -1;
  }

<<<<<<< HEAD
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
=======
  // This will give us the specific bit where we found the free block
  // in the specific int block
  bitNum = 32 - bitNum;

  int index = bitNum;

  // We want to start from the bitnum (index) and go up to the bitNum + 
  // blocksAllocated while setting the bits to 0, representing that the
  // corresponding blocks are used
  int total = (bitNum + blocksAllocated); 

  for (; index < total; index++) {
    // If we have reached the end of an integer, we move on to the next
    // integer, and reset the index
    if (index > 32) {
      intBlock += 1;
      index = 1;
      total -= 32;
    }

    // We first create a bit mask by shifting 1 to the left, to a
    // position we want to clear the bit at, calculated using (32 - index)
    // then we use the not (~) operator to clear the bit at the position given
    // by (32 - index) and then we apply the mask to our integer, which will 
    // basically clear the corresponding bit, since 1 & 0 = 0
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    bitVector[intBlock] = bitVector[intBlock] & ~(1 << (32 - index));
  }

  LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, 1);
}


<<<<<<< HEAD
void setBlocksAsFree(int freeBlock, int blocksFreed) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

  // Set the number of bits specified in the blocksFreed
  // to 1 starting from freeBlock
=======
//Updates the free space bit vector with freed blocks
void setBlocksAsFree(int freeBlock, int blocksFreed) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
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
<<<<<<< HEAD
  int total = (bitNum + blocksFreed);

  for (; index < total; index++) {
=======

  // We want to start from the bitnum (index) and go up to the bitNum + 
  // blocksAllocated while setting the bits to 1, representing that the
  // corresponding blocks are free
  int total = (bitNum + blocksFreed);

  for (; index < total; index++) {
    // If we have reached the end of an integer, we move on to the next
    // integer, and reset the index
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    if (index > 32) {
      intBlock += 1;
      index = 1;
      total -= 32;
    }
<<<<<<< HEAD
=======

    // We first create a bit mask by shifting 1 to the left, to a
    // position we want to set the bit at, calculated using (32 - index)
    // then we apply the mask to our integer, which will 
    // basically set the corresponding bit
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    bitVector[intBlock] = bitVector[intBlock] | (1 << (32 - index));
  }

  LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, 1);
}

<<<<<<< HEAD
/****************************************************
*  fs_stat
****************************************************/
int fs_stat(const char* path, struct fs_stat* buf){
  // fs_stat() displays file details associated with the file system

  // printf("************* Entering fs_stat() **************\n");

  // *** Validation Checks ***
  //printf("*** Validation Checks ***\n");
  if (path == NULL){
    printf("fs_stat(): Path cannot be null.\n");
    return -1;
  }else{
    //printf("fs_stat(): const char* path is: %s\n", path);
  }
  
  char* pathCopy = malloc(sizeof(path));

  if(pathCopy == NULL){
    printf("fs_stat(): Memory allocation error.\n");
    return -1;
  }else{
    //printf("fs_stat(): Memory allocation is successful. pathCopy is not null.\n");
  }

  // printf("fs_stat(): Validity checks finished.\n\n");
  
  // *** Store information ***
  // printf("*** Store information ***\n");


  // Create a char* from const char* in order to manipulate path
  strcpy(pathCopy, path);
  // printf("fs_stat(): strcpy() successful. pathCopy is %s\n", pathCopy);
  // printf("fs_stat(): Checking for hash value: %d\n", hash(pathCopy));
=======

//Prints out the details of a directory entry
int fs_stat(const char* path, struct fs_stat* buf) {
  // fs_stat() displays file details associated with the file system

  // *** Validation Checks ***
  if (path == NULL) {
    return -1;
  }

  char* pathCopy = malloc(sizeof(path));
  if (!pathCopy) {
    mallocFailed();
  }

  // Create a char* from const char* in order to manipulate path
  strcpy(pathCopy, path);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  // Handle the case of when an absolute path is given
  char** parsedPath = stringParser(pathCopy);
  char* desiredPath;
  int i = 0;
<<<<<<< HEAD
  while(parsedPath[i] != NULL){
    desiredPath = parsedPath[i];
    i++;
  }
  // printf("desiredPath: %s\n", desiredPath);
=======
  while (parsedPath[i] != NULL) {
    desiredPath = parsedPath[i];
    i++;
  }
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  // Pull desired path into memory
  hashTable* currentDirTbl = readTableData(workingDir->location);
  dirEntry* currentEntry = getEntry(desiredPath, currentDirTbl);
<<<<<<< HEAD
  if(currentEntry == NULL){
    printf("fs_stat(): getEntry() failed. Could not find path within fs.\n");
    return -1;
  }
  // printf("Current Entry Filename: %s\n", currentEntry->filename);

  // printf("\n\n\n***********fs_stat() BEGINING OUTPUT *********\n");
=======

  if (currentEntry == NULL) {
    return -1;
  }
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  printf("File: \t%s\n", currentEntry->filename);

  buf->st_size = currentEntry->fileSize;
  printf("Size: \t%ld\n", buf->st_size);

<<<<<<< HEAD
  buf->st_blksize = 512;
  printf("IO Block size: \t%d\n", buf->st_blksize);

  buf->st_blocks = currentEntry->fileSize / 512;
=======
  buf->st_blksize = blockSize;
  printf("IO Block size: \t%ld\n", buf->st_blksize);

  buf->st_blocks = currentEntry->fileSize / blockSize;
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  printf("Blocks: \t%ld\n", buf->st_blocks);

  // Create variables for time
  time_t currentTime;
  struct tm ts;
  char time_buf[80];

<<<<<<< HEAD
  // time_t now;   
  // struct tm *local = localtime(&now);

  // This returns the current time
  time(&currentTime);
  //printf("Current time is \t%ld\n", time(&currentTime));
  // Adjust to local time and format into YYYY-MM-DD HH:MM:SS TIMEZONE
  ts = *localtime(&currentTime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  //printf("Current time formatted: \t%s\n", time_buf);

  // Store epoch time, but print out formatted time
  buf->st_accesstime = (long int)ctime(&currentTime) / 60;
=======
  time(&currentTime);

  // Store epoch time, but print out formatted time
  buf->st_accesstime = currentEntry->dateModified;
  ts = *localtime(&buf->st_accesstime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  printf("Access Time: \t%s\n", time_buf);

  buf->st_modtime = currentEntry->dateModified;
  ts = *localtime(&buf->st_modtime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Modtime: \t%s\n", time_buf);

  buf->st_createtime = currentEntry->dateCreated;
  ts = *localtime(&buf->st_createtime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Create Time: \t%s\n", time_buf);
<<<<<<< HEAD

  // printf("\n\n\n***********fs_stat() END OUTPUT *********\n");
  // LBAwrite();
  return 0;
}

=======
  return 0;
}


>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
//Check if a path is a directory (1 = yes, 0 = no)
int fs_isDir(char* path) {
  int result = isDirWithValidPath(path);
  return result == -1 ? 0 : result;
}


// Check if a path is a file (1 = yes, 0 = no)
int fs_isFile(char* path) {
  int result = isDirWithValidPath(path);
  return result == -1 ? 0 : !result;
}


// Opens a directory stream corresponding to 'name', and returns
// a pointer to the directory stream
fdDir* fs_opendir(const char* name) {
  fdDir* fdDir = malloc(sizeof(fdDir));
  if (!fdDir) {
    mallocFailed();
  }
<<<<<<< HEAD
  dirEntry* reqDir = getEntry((char*)name, workingDir);
  hashTable* reqDirTable = readTableData(reqDir->location);

  fdDir->dirTable = reqDirTable;
  fdDir->maxIdx = reqDirTable->maxNumEntries;
  fdDir->d_reclen = reqDirTable->numEntries;
  fdDir->directoryStartLocation = reqDir->location;
  fdDir->dirEntryPosition = reqDirTable->maxNumEntries;
=======

  hashTable* dir = getDir((char*)name);

  fdDir->dirTable = dir;
  fdDir->maxIdx = dir->maxNumEntries;
  fdDir->d_reclen = dir->numEntries;
  fdDir->directoryStartLocation = dir->location;
  fdDir->dirEntryPosition = dir->maxNumEntries;
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

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

<<<<<<< HEAD

struct fs_diriteminfo* fs_readdir(fdDir* dirp) {
  printf("Inside ReadDir numEntries: %d\n", dirp->dirTable->numEntries);
  //Calculate the new hash table index to use and save it to the fdDir
  int dirEntIdx = getNextIdx(dirp->dirEntryPosition, dirp->dirTable);

=======
//Moves to the next entry in the directory associated with dirp and
//returns its info
struct fs_diriteminfo* fs_readdir(fdDir* dirp) {
  //Calculate the new hash table index to use and save it to the fdDir
  int dirEntIdx = getNextIdx(dirp->dirEntryPosition, dirp->dirTable);

  static int prevIdx = -999;
  static int prevIdxCount = 0;

  if (prevIdx == dirEntIdx) {
    prevIdxCount++;
  } else {
    prevIdxCount = 0;
  }


>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  if (dirEntIdx == dirp->maxIdx) {
    return NULL;
  }

<<<<<<< HEAD
  printf("FOUND INDEX: %d\n", dirEntIdx);

=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  dirp->dirEntryPosition = dirEntIdx;

  hashTable* dirTable = dirp->dirTable;
  dirEntry* dirEnt = dirTable->entries[dirEntIdx]->value;

<<<<<<< HEAD
=======
  for (int i = prevIdxCount; i > 0; i--) {
    dirEnt = dirTable->entries[dirEntIdx]->next->value;
  }

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  //Create and populate the directory item info pointer
  struct fs_diriteminfo* dirItemInfo = malloc(sizeof(struct fs_diriteminfo));
  if (!dirItemInfo) {
    mallocFailed();
  }

  strcpy(dirItemInfo->d_name, dirEnt->filename);
  dirItemInfo->d_reclen = dirEnt->fileSize;
  dirItemInfo->fileType = dirEnt->isDir ? 'd' : 'f';
<<<<<<< HEAD
  printf("Inside ReadDir numEntries: %d\n", dirp->dirTable->numEntries);
=======
  prevIdx = dirEntIdx;
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  return dirItemInfo;

}


hashTable* getDir(char* buf) {
  if (fs_isDir(buf)) {
    //Parse path
    char** parsedPath = stringParser(buf);
    int fullPath = strcmp(parsedPath[0], "/") == 0;

    hashTable* currDir;
    if (fullPath) {  //Absolute path
      struct volumeCtrlBlock* vcbPtr = malloc(blockSize);
      if (!vcbPtr) {
        mallocFailed();
      }
      LBAread(vcbPtr, 1, 0);
      currDir = readTableData(vcbPtr->rootDir);
      free(vcbPtr);
      vcbPtr = NULL;
    } else {  //Relative path
      currDir = readTableData(workingDir->location);
    }

    //Continue until we have processed each component in the path
    int i = 0;
    if (fullPath) {
      i++;
    }

    dirEntry* entry;
    for (; parsedPath[i] != NULL; i++) {
      //check that the location exists and that it is a directory
      entry = getEntry(parsedPath[i], currDir);
      free(currDir);
      currDir = readTableData(entry->location);
    }

    return readTableData(currDir->location);

  } else {
<<<<<<< HEAD
    printf("From getDir, isDir returned false\n");
=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    return NULL;
  }
}


char** stringParser(char* inputStr) {
  // Divide the path provided by the user into
  // several sub paths
  char* stringToParse = malloc(strlen(inputStr) + 1);
  if (!stringToParse) {
    mallocFailed();
  }

  strcpy(stringToParse, inputStr);

  char** subStrings = (char**)malloc(sizeof(char*) * (strlen(stringToParse) + 1));
  if (!subStrings) {
    mallocFailed();
  }

  char* subString;
  char* savePtr;
  char* delim = "/";

  int stringCount = 0;
  //Check if root directory
  if (stringToParse[0] == '/') {
    subStrings[stringCount] = "/";
    stringCount++;
  }

  subString = strtok_r(stringToParse, delim, &savePtr);


  while (subString != NULL) {
    subStrings[stringCount] = subString;
<<<<<<< HEAD
    // printf("substring: %s at i = %d\n", subString, stringCount);
=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    stringCount++;
    subString = strtok_r(NULL, delim, &savePtr);
  }

  subStrings[stringCount] = subString;


  return subStrings;
}

<<<<<<< HEAD

=======
//Sets the current working directory to the directory specified by 
//buf if it exists
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
int fs_setcwd(char* buf) {
  hashTable* requestedDir = getDir(buf);

  if (requestedDir == NULL) {
<<<<<<< HEAD
    printf("From setcwd, isDir returned false\n");
=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    return -1;
  }

  workingDir = requestedDir;
<<<<<<< HEAD
  printf("Leaving setcwd with working dir %s\n", workingDir->dirName);
  return 0;
}


=======
  return 0;
}

//Gets the current working directory
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
char* fs_getcwd(char* buf, size_t size) {
  char* path = malloc(size);
  if (!path) {
    mallocFailed();
  }

  path[0] = '/';
  path[1] = '\0';

  //Check if cwd is root
  if (strcmp(workingDir->dirName, "/") == 0) {
    strcpy(buf, path);
<<<<<<< HEAD
    printf("getcwd: final working dir %s\n", workingDir->dirName);
    return path;
  }

  printf("After strcmp()\n");

  char* pathElements[size];
  int i = 0;

  printf("After checking cwd is root\n");

=======
    return path;
  }

  char* pathElements[size];
  int i = 0;

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  hashTable* currDir = readTableData(workingDir->location);
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

<<<<<<< HEAD
  // free(path);
  // path = NULL;
  printf("getcwd: final working dir %s\n", workingDir->dirName);
  if (workingDir->dirName == NULL){
    printf("getcwd: final working dir is null!\n");
    return "";
  }
  return buf;
}


=======
  return buf;
}

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
//Creates a new directory
int fs_mkdir(const char* pathname, mode_t mode) {
  deconPath* pathParts = splitPath((char*)pathname);
  char* parentPath = pathParts->parentPath;

<<<<<<< HEAD
  if (!fs_isDir(parentPath) || fs_isDir((char*)pathname)) {
=======
  if (!fs_isDir(parentPath)) {
    printf("md: cannot create directory '%s': No such file or directory\n", pathname);
    return -1;
  } else if (fs_isDir((char*)pathname)) {
    printf("md: cannot create directory '%s': File exists\n", pathname);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    return -1;
  }

  hashTable* parentDir = getDir(parentPath);
<<<<<<< HEAD
  printf("Parent dir is %s\n", parentDir->dirName);
=======

  // Create a new directory entry
  char* newDirName = pathParts->childName;
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  int sizeOfEntry = sizeof(dirEntry);	//48 bytes
  int dirSizeInBytes = (DIR_SIZE * blockSize);	//2560 bytes
  int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

  // Get the bitVector in memory -- We need to know what
  // block is free so we can store our new directory
  // there
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, 1);

<<<<<<< HEAD
  // Create a new directory entry
  char* newDirName = pathParts->childName;

=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  dirEntry* newEntry = malloc(sizeof(dirEntry));
  if (!newEntry) {
    mallocFailed();
  }
<<<<<<< HEAD
  int freeBlock = getFreeBlockNum();
=======

  int freeBlock = getFreeBlockNum(DIR_SIZE);
  // Check if the freeBlock returned is valid or not
  if (freeBlock < 0) {
    return -1;
  }
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  // Initialize the new directory entry
  strcpy(newEntry->filename, newDirName);
  newEntry->isDir = 1;
  newEntry->location = freeBlock;
  newEntry->fileSize = DIR_SIZE * blockSize;
  newEntry->dateModified = time(0);
  newEntry->dateCreated = time(0);

  // Put the updated directory entry back
  // into the directory
  setEntry(newDirName, newEntry, parentDir);

  // Initialize the directory entries within the new
  // directory
  int startBlock = getEntry(newDirName, parentDir)->location;
  hashTable* dirEntries = hashTableInit(newDirName, maxNumEntries, startBlock);

  // Initializing the "." current directory and the ".." parent Directory
  dirEntry* currDirEnt = dirEntryInit(".", 1, freeBlock,
    dirSizeInBytes, time(0), time(0));
  setEntry(currDirEnt->filename, currDirEnt, dirEntries);

  dirEntry* parentDirEnt = dirEntryInit("..", 1, parentDir->location,
    dirSizeInBytes, time(0), time(0));
  setEntry(parentDirEnt->filename, parentDirEnt, dirEntries);

  // Write parent directory
  writeTableData(parentDir, parentDir->location);
  // Write new directory
  writeTableData(dirEntries, dirEntries->location);

  // Update the bit vector
<<<<<<< HEAD
  // printf("NEW FREE BLOCK: %d\n", freeBlock);
  setBlocksAsAllocated(freeBlock, DIR_SIZE);
  printTable(parentDir);

  printf("Freeing\n");
=======
  setBlocksAsAllocated(freeBlock, DIR_SIZE);

>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  free(bitVector);
  bitVector = NULL;
  free(newEntry);
  newEntry = NULL;
  free(pathParts);
  pathParts = NULL;

  return 0;
}

<<<<<<< HEAD

=======
//Removes a directory if its empty
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
int fs_rmdir(const char* pathname) {
  deconPath* pathParts = splitPath((char*)pathname);
  char* parentPath = pathParts->parentPath;

  if (!fs_isDir((char*)pathname)) {
<<<<<<< HEAD
=======
    printf("rm: cannot remove directory '%s': directory does not exist\n", pathname);
    return -1;
  } else if (strcmp(pathname, "/") == 0) {
    printf("rm: cannot remove directory '%s': directory is root\n", pathname);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    return -1;
  }

  hashTable* parentDir = getDir(parentPath);

  int sizeOfEntry = sizeof(dirEntry);	//48 bytes
  int dirSizeInBytes = (DIR_SIZE * blockSize);	//2560 bytes
  int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

  // Get the bitVector in memory -- We need to know what
  // block is free so we can store our new directory
  // there
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, 1);

  char* dirNameToRemove = pathParts->childName;
  int dirToRemoveLocation = getEntry(dirNameToRemove, parentDir)->location;
  hashTable* dirToRemove = readTableData(dirToRemoveLocation);

<<<<<<< HEAD
  //Check if empty
  if (dirToRemove->numEntries > 2) {
    printf("Directory is not empty, refusing to delete\n");
=======
  //Get the working directory's parent directory
  char* cwdParentPath = malloc(100);
  fs_getcwd(cwdParentPath, 100);
  hashTable* cwdParent = getDir(strcat(cwdParentPath, ".."));

  //Don't delete if it is the '.' or '..' directory
  if (strcmp(dirToRemove->dirName, workingDir->dirName) == 0 ||
    strcmp(dirToRemove->dirName, cwdParent->dirName) == 0) {
    printf("rm: cannot remove directory '.' or '..'\n");
    free(cwdParentPath);
    cwdParentPath = NULL;
    free(cwdParent);
    cwdParent = NULL;
    return -1;
  }

  free(cwdParentPath);
  cwdParentPath = NULL;
  free(cwdParent);
  cwdParent = NULL;

  //Check if empty
  if (dirToRemove->numEntries > 2) {
    printf("rm: cannot remove directory '%s': directory is not empty\n", pathname);
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
    return -1;
  }

  //Remove dirEntry from the parent dir
  rmEntry(dirNameToRemove, parentDir);

  //Rewrite parent dir to disk
  writeTableData(parentDir, parentDir->location);

  //Update the free space bit vector
  setBlocksAsFree(dirToRemoveLocation, DIR_SIZE);

<<<<<<< HEAD
  printf("Freeing\n");
=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  free(pathParts);
  pathParts = NULL;

  return 0;
}

<<<<<<< HEAD

int fs_delete(char* filename) {
  deconPath* pathParts = splitPath((char*)filename);
  char* parentPath = pathParts->parentPath;

  if (!fs_isDir((char*)filename)) {
    return -1;
  }

  hashTable* parentDir = getDir(parentPath);

  int sizeOfEntry = sizeof(dirEntry);	//48 bytes
  int dirSizeInBytes = (DIR_SIZE * blockSize);	//2560 bytes
  int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

  // Get the bitVector in memory -- We need to know what
  // block is free so we can store our new directory
  // there
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, 1);

  char* fileNameToRemove = pathParts->childName;
  int fileToRemoveLocation = getEntry(fileNameToRemove, parentDir)->location;

  //Iterate over each file block to get all associated block nums

  //Set each block num as free
=======
//Removes a file
int fs_delete(char* filename) {
  // Split the path into parent and child component
  deconPath* pathParts = splitPath((char*)filename);
  hashTable* parentDir = getDir(pathParts->parentPath);

  char* fileNameToRemove = pathParts->childName;
  dirEntry* dirEntry = getEntry(pathParts->childName, parentDir);
  int fileToRemoveLocation = dirEntry->location;

  int blockToFree = dirEntry->location;

  //Iterate over each file block to get all associated block nums
  while (blockToFree) {
    //Set each block num as free
    setBlocksAsFree(blockToFree, 1);

    char* buffer = malloc(blockSize);
    LBAread(buffer, 1, blockToFree);

    char blockChars[6];

    for (int j = 0; j < 5; j++) {
      blockChars[j] = buffer[j];
    }
    blockChars[5] = '\0';

    // Convert the characters representing block number to
    // an integer
    const char* constBlockNumbs = blockChars;
    blockToFree = atoi(constBlockNumbs);
  }
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440

  //Remove dirEntry from the parent dir
  rmEntry(fileNameToRemove, parentDir);

  //Rewrite parent dir to disk
  writeTableData(parentDir, parentDir->location);

<<<<<<< HEAD
  printf("Freeing\n");
=======
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
  free(pathParts);
  pathParts = NULL;

  return 0;
}

