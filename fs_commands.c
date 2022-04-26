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

//Read all directory entries from a certain disk location into a new hashmap
hashTable* readTableData(int lbaPosition) {
  int arrNumBytes = ((DIR_SIZE * blockSize) / sizeof(dirEntry)) * sizeof(dirEntry);

  //All table data
  typedef struct tableData {
    char dirName[20];
    dirEntry arr[arrNumBytes];
  } tableData;

  //Read all of the entries into an array
  tableData* data = malloc(DIR_SIZE * blockSize);
  if (!data) {
    mallocFailed();
  }
  LBAread(data, DIR_SIZE, lbaPosition);

  dirEntry* arr = data->arr;

  //Create a new hash table to be populated
  hashTable* dirPtr = hashTableInit(data->dirName, ((DIR_SIZE * blockSize) / sizeof(dirEntry) - 1),
    lbaPosition);

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


//Write all directory entries in the hashTable to the disk
void writeTableData(hashTable* table, int lbaPosition) {
  int arrNumBytes = table->maxNumEntries * sizeof(dirEntry);

  //All table data
  typedef struct tableData {
    char dirName[20];
    dirEntry arr[arrNumBytes];
  } tableData;

  tableData* data = calloc(blockSize * DIR_SIZE, 1);
  if (!data) {
    mallocFailed();
  }

  //Directory entries
  dirEntry* arr = calloc(arrNumBytes, 1);
  if (!arr) {
    mallocFailed();
  }

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

  memcpy(data->arr, arr, arrNumBytes);

  //Write to the array out to the specified block numbers
  int val = LBAwrite(data, DIR_SIZE, lbaPosition);


  clean(table);
  table = NULL;
  free(arr);
  arr = NULL;
  free(data);
  data = NULL;
}


//Check if a path is a directory (1 = yes, 0 = no, -1 = error in parent path)
int isDirWithValidPath(char* path) {
  char** parsedPath = stringParser(path);
  int absPath = strcmp(parsedPath[0], "/") == 0;

  //Check if path is root, empty, or multiple '/'
  if (parsedPath[0] == NULL || (absPath && parsedPath[1] == NULL)) {
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
    currDir = readTableData(workingDir->location);
  }

  int i = 0;

  if (absPath) {
    i++;
  }

  dirEntry* entry;

  for (; parsedPath[i + 1] != NULL; i++) {
    //check that the location exists and that it is a directory
    entry = getEntry(parsedPath[i], currDir);

    if (entry == NULL || entry->isDir == 0) {
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


  free(parsedPath);
  parsedPath = NULL;
  free(parentPath);
  parentPath = NULL;

  if (entry == NULL) {
    return -1;
  }

  int result = entry->isDir;

  free(currDir);
  currDir = NULL;

  return result;
}


deconPath* splitPath(char* fullPath) {
  char** parsedPath = stringParser(fullPath);
  char* parentPath = malloc(strlen(fullPath) + 1);
  if (!parentPath) {
    mallocFailed();
  }

  int k = 0;
  int i = 0;
  for (; parsedPath[i + 1] != NULL; i++) {
    for (int j = 0; j < strlen(parsedPath[i]); j++) {
      parentPath[k] = parsedPath[i][j];
      k++;
    }

    if (parentPath[k - 1] != '/') {
      parentPath[k] = '/';
      k++;
    }
  }

  parentPath[k] = '\0';

  deconPath* pathParts = malloc(sizeof(deconPath));
  if (!pathParts) {
    mallocFailed();
  }
  pathParts->parentPath = parentPath;
  pathParts->childName = parsedPath[i];

  if (strcmp(pathParts->parentPath, "") == 0) {
    pathParts->parentPath = ".";
  } else if (strcmp(pathParts->childName, "") == 0) {
    pathParts->childName = pathParts->parentPath;
    pathParts->parentPath = ".";
  }

  return pathParts;
}

// This will help us determine the int block in which we
// found a bit of value 1 representing free block
int intBlock = 0;

int getFreeBlockNum(int getNumBlocks) {
  //**********Get the free block number ***********
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

  // This will help determine the first block number that is
  // free
  int freeBlock = -1;
  int blocksToFind = getNumBlocks;

  //****Calculate free space block number*****
  // We can use the following formula to calculate the block
  // number => (32 * i) + (32 - j), where (32 * i) will give us 
  // the number of 32 bit blocks where we found a bit of value 1
  // and we add (31 - j) which is a offset to get the block number 
  // it represents within that 32 bit block
  for (int i = 0; i < numOfInts; i++) {
    for (int j = 31; j >= 0; j--) {
      if (bitVector[i] & (1 << j)) {
        blocksToFind--;

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
      // block that is not free after finding a block that was free
      else if (freeBlock != -1) {
        freeBlock = -1;
        blocksToFind = getNumBlocks;
      }
    }
  }

  printf("Error: Couldn't find %d contiguous free blocks\n", getNumBlocks);
  return -1;
}


void setBlocksAsAllocated(int freeBlock, int blocksAllocated) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

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


void setBlocksAsFree(int freeBlock, int blocksFreed) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

  // Set the number of bits specified in the blocksFreed
  // to 1 starting from freeBlock
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
  int total = (bitNum + blocksFreed);

  for (; index < total; index++) {
    if (index > 32) {
      intBlock += 1;
      index = 1;
      total -= 32;
    }
    bitVector[intBlock] = bitVector[intBlock] | (1 << (32 - index));
  }

  LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, 1);
}


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

  // Handle the case of when an absolute path is given
  char** parsedPath = stringParser(pathCopy);
  char* desiredPath;
  int i = 0;
  while (parsedPath[i] != NULL) {
    desiredPath = parsedPath[i];
    i++;
  }

  // Pull desired path into memory
  hashTable* currentDirTbl = readTableData(workingDir->location);
  dirEntry* currentEntry = getEntry(desiredPath, currentDirTbl);

  if (currentEntry == NULL) {
    return -1;
  }

  printf("File: \t%s\n", currentEntry->filename);

  buf->st_size = currentEntry->fileSize;
  printf("Size: \t%ld\n", buf->st_size);

  buf->st_blksize = blockSize;
  printf("IO Block size: \t%ld\n", buf->st_blksize);

  buf->st_blocks = currentEntry->fileSize / blockSize;
  printf("Blocks: \t%ld\n", buf->st_blocks);

  // Create variables for time
  time_t currentTime;
  struct tm ts;
  char time_buf[80];

  time(&currentTime);

  // Store epoch time, but print out formatted time
  buf->st_accesstime = currentEntry->dateModified;
  ts = *localtime(&buf->st_accesstime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Access Time: \t%s\n", time_buf);

  buf->st_modtime = currentEntry->dateModified;
  ts = *localtime(&buf->st_modtime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Modtime: \t%s\n", time_buf);

  buf->st_createtime = currentEntry->dateCreated;
  ts = *localtime(&buf->st_createtime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Create Time: \t%s\n", time_buf);
  return 0;
}

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

  hashTable* dir = getDir((char*)name);

  fdDir->dirTable = dir;
  fdDir->maxIdx = dir->maxNumEntries;
  fdDir->d_reclen = dir->numEntries;
  fdDir->directoryStartLocation = dir->location;
  fdDir->dirEntryPosition = dir->maxNumEntries;

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


  if (dirEntIdx == dirp->maxIdx) {
    return NULL;
  }

  dirp->dirEntryPosition = dirEntIdx;

  hashTable* dirTable = dirp->dirTable;
  dirEntry* dirEnt = dirTable->entries[dirEntIdx]->value;

  for (int i = prevIdxCount; i > 0; i--) {
    dirEnt = dirTable->entries[dirEntIdx]->next->value;
  }

  //Create and populate the directory item info pointer
  struct fs_diriteminfo* dirItemInfo = malloc(sizeof(struct fs_diriteminfo));
  if (!dirItemInfo) {
    mallocFailed();
  }

  strcpy(dirItemInfo->d_name, dirEnt->filename);
  dirItemInfo->d_reclen = dirEnt->fileSize;
  dirItemInfo->fileType = dirEnt->isDir ? 'd' : 'f';
  prevIdx = dirEntIdx;
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
    stringCount++;
    subString = strtok_r(NULL, delim, &savePtr);
  }

  subStrings[stringCount] = subString;


  return subStrings;
}

//Sets the current working directory to the directory specified by 
//buf if it exists
int fs_setcwd(char* buf) {
  hashTable* requestedDir = getDir(buf);

  if (requestedDir == NULL) {
    return -1;
  }

  workingDir = requestedDir;
  return 0;
}

//Gets the current working directory
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
    return path;
  }

  char* pathElements[size];
  int i = 0;

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

  return buf;
}

//Creates a new directory
int fs_mkdir(const char* pathname, mode_t mode) {
  deconPath* pathParts = splitPath((char*)pathname);
  char* parentPath = pathParts->parentPath;

  if (!fs_isDir(parentPath)) {
    printf("md: cannot create directory '%s': No such file or directory\n", pathname);
    return -1;
  } else if (fs_isDir((char*)pathname)) {
    printf("md: cannot create directory '%s': File exists\n", pathname);
    return -1;
  }

  hashTable* parentDir = getDir(parentPath);

  // Create a new directory entry
  char* newDirName = pathParts->childName;

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

  dirEntry* newEntry = malloc(sizeof(dirEntry));
  if (!newEntry) {
    mallocFailed();
  }

  int freeBlock = getFreeBlockNum(DIR_SIZE);
  // Check if the freeBlock returned is valid or not
  if (freeBlock < 0) {
    return -1;
  }

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
  setBlocksAsAllocated(freeBlock, DIR_SIZE);

  free(bitVector);
  bitVector = NULL;
  free(newEntry);
  newEntry = NULL;
  free(pathParts);
  pathParts = NULL;

  return 0;
}

//Removes a directory if its empty
int fs_rmdir(const char* pathname) {
  deconPath* pathParts = splitPath((char*)pathname);
  char* parentPath = pathParts->parentPath;

  if (!fs_isDir((char*)pathname)) {
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

  //Check if empty
  if (dirToRemove->numEntries > 2) {
    printf("Directory is not empty, refusing to delete\n");
    return -1;
  }

  //Remove dirEntry from the parent dir
  rmEntry(dirNameToRemove, parentDir);

  //Rewrite parent dir to disk
  writeTableData(parentDir, parentDir->location);

  //Update the free space bit vector
  setBlocksAsFree(dirToRemoveLocation, DIR_SIZE);

  free(pathParts);
  pathParts = NULL;

  return 0;
}

//Removes a file
int fs_delete(char* filename) {
  deconPath* pathParts = splitPath((char*)filename);
  hashTable* parentDir = getDir(pathParts->parentPath);

  char* fileNameToRemove = pathParts->childName;
  dirEntry* dirEntry = getEntry(pathParts->childName, parentDir);
  int fileToRemoveLocation = dirEntry->location;

  //Iterate over each file block to get all associated block nums
  int blockToFree = dirEntry->location;

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

  //Remove dirEntry from the parent dir
  rmEntry(fileNameToRemove, parentDir);

  //Rewrite parent dir to disk
  writeTableData(parentDir, parentDir->location);

  free(pathParts);
  pathParts = NULL;

  return 0;
}

