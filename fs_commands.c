#include "fs_commands.h"

void mallocFailed() {
  printf("Call to malloc memory failed, exiting program...\n");
  exit(-1);
}

/****************************************************
*  readTableData
****************************************************/
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

void setBlocksAsFree(int freeBlock, int blocksAllocated) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

  // Set the number of bits specified in the blocksAllocated
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
  int sumOfFreeBlAndBlocksAlloc = (bitNum + blocksAllocated);

  for (; index < sumOfFreeBlAndBlocksAlloc; index++) {
    if (index > 32) {
      intBlock += 1;
      index = 1;
      sumOfFreeBlAndBlocksAlloc -= 32;
    }
    bitVector[intBlock] = bitVector[intBlock] | (1 << (32 - index));
  }

  LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, 1);
}


/****************************************************
*  writeTableData
****************************************************/

//Write all directory entries in the hashTable to the disk
void writeTableData(hashTable* table, int lbaPosition) {
  int arrNumBytes = table->maxNumEntries * sizeof(dirEntry);

  //All table data
  typedef struct tableData {
    char dirName[20];
    dirEntry arr[arrNumBytes];
  } tableData;

  tableData* data = malloc(blockSize * DIR_SIZE);
  if (!data) {
    mallocFailed();
  }

  //Directory entries
  dirEntry* arr = malloc(arrNumBytes);
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
    // printf("Getting entry\n");
    entry = getEntry(parsedPath[i], currDir);
    // printf("Entry is %s\n", entry->filename);

    if (entry == NULL || entry->isDir == 0) {
      printf("Error: Part of parent path does not exist\n");
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
    return 0;
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

    if (parentPath[k - 1] != '/' && parentPath[k - 1] != '.') {
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


/****************************************************
*  fs_stat
****************************************************/
int fs_stat(const char* path, struct fs_stat* buf) {
  // // fs_stat() displays details associated with the file system

  // printf("************* Entering fs_stat() **************\n");

  int returnVal = 0;
  // time_t now;
  // struct tm* local = localtime(&now);

  // // *** Validation Checks ***
  // printf("*** Validation Checks ***\n");
  // if (path == NULL) {
  //   printf("fs_stat(): Path cannot be null.\n");
  //   return -1;
  // } else {
  //   printf("fs_stat(): const char* path is: %s\n", path);
  // }

  // char* pathCopy = malloc(sizeof(path));

  // if (pathCopy == NULL) {
  //   printf("fs_stat(): Memory allocation error.\n");
  //   return -1;
  // } else {
  //   printf("fs_stat(): Memory allocation is successful. pathCopy is not null.\n");
  // }

  // printf("fs_stat(): Validity checks finished.\n\n");

  // // *** Store information ***
  // printf("*** Store information ***\n");
  // strcpy(pathCopy, path);
  // printf("fs_stat(): strcpy() successful. pathCopy is %s\n", pathCopy);
  // printf("fs_stat(): Checking for hash value: %d\n", hash(pathCopy));
  // hashTable* currentDirTbl;
  // // currentDirTbl = readTableData(workingDir->location);
  // // dirEntry* currentEntry getEntry(pathCopy, currentDirTbl);
  // // printf("Current Entry Filename: %s\n", currentEntry->filename);

  // printf("Path: %s\n", path);
  // printf("Size: %ld\n", buf->st_size);
  // printf("Block size: %d\n", buf->st_blksize);
  // printf("Blocks: %ld\n", buf->st_blocks);
  // // YYYY-MM-DD HH:MM:SS TIMEZONE

  // time(&now);
  // buf->st_accesstime = (long int)ctime(&now);
  // printf("Access Time: %ld\n", buf->st_accesstime);

  // // buf->st_modtime = currEntry->dateModified;
  // printf("Modtime: %ld\n", buf->st_modtime);



  // // buf->st_createtime = currEntry->dateCreated;
  // // struct tm ts;
  // // char  buf[80];

  // // ts = *localtime(currEntry->dateCreated);
  // // char * convertTime(time_t epochTime){
  // //   int epochTimeInt = (int)epochTime; 

  // //   long long int timeInSec = (ep)
  // // }

  // printf("Create Time: %ld\n", buf->st_createtime);

  //How to write to disk
  return returnVal;
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
  if (!dirItemInfo) {
    mallocFailed();
  }

  strcpy(dirItemInfo->d_name, dirEnt->filename);
  dirItemInfo->d_reclen = dirEnt->fileSize;
  dirItemInfo->fileType = dirEnt->isDir ? 'd' : 'f';
  printf("Inside ReadDir numEntries: %d\n", dirp->dirTable->numEntries);
  return dirItemInfo;

}


hashTable* getDir(char* buf) {
  if (fs_isDir(buf)) {
    //Parse path
    char** parsedPath = stringParser(buf);
    int fullPath = strcmp(parsedPath[0], "/") == 0;

    hashTable* currDir;
    if (fullPath) {  //Absolute path
      // struct volumeCtrlBlock* vcbPtr = malloc(blockSize);
      // if (!vcbPtr) {
      //   mallocFailed();
      // }
      // LBAread(vcbPtr, 1, 0);
      // currDir = readTableData(vcbPtr->rootDir);
      // free(vcbPtr);
      // vcbPtr = NULL;
      currDir = getDir("/");
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
    printf("From getDir, isDir returned false\n");
    return NULL;
  }
}

/****************************************************
*  stringParser
****************************************************/
// Helper functions
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
    // printf("substring: %s at i = %d\n", subString, stringCount);
    stringCount++;
    subString = strtok_r(NULL, delim, &savePtr);
  }

  subStrings[stringCount] = subString;


  return subStrings;
}



int fs_setcwd(char* buf) {
  hashTable* requestedDir = getDir(buf);

  if (requestedDir == NULL) {
    printf("From setcwd, isDir returned false\n");
    return -1;
  }

  workingDir = requestedDir;
  printf("Leaving setcwd with working dir %s\n", workingDir->dirName);
  return 0;
}

/****************************************************
*  fs_getcwd
****************************************************/
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
    printf("getcwd: final working dir %s\n", workingDir->dirName);
    return path;
  }

  printf("After strcmp()\n");

  char* pathElements[size];
  int i = 0;

  printf("After checking cwd is root\n");

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

  // free(path);
  // path = NULL;
  printf("getcwd: final working dir %s\n", workingDir->dirName);
  return buf;
}


//Creates a new directory
int fs_mkdir(const char* pathname, mode_t mode) {
  deconPath* pathParts = splitPath((char*)pathname);
  char* parentPath = pathParts->parentPath;

  if (!fs_isDir(parentPath) || fs_isDir((char*)pathname)) {
    return -1;
  }

  hashTable* parentDir = getDir(parentPath);
  printf("Parent dir is %s\n", parentDir->dirName);

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

  // Create a new directory entry
  char* newDirName = pathParts->childName;

  dirEntry* newEntry = malloc(sizeof(dirEntry));
  if (!newEntry) {
    mallocFailed();
  }
  int freeBlock = getFreeBlockNum();

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
  // printf("NEW FREE BLOCK: %d\n", freeBlock);
  setBlocksAsAllocated(freeBlock, DIR_SIZE);
  printTable(parentDir);

  printf("Freeing\n");
  free(bitVector);
  bitVector = NULL;
  free(newEntry);
  newEntry = NULL;
  free(pathParts);
  pathParts = NULL;

  return 0;
}

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

  printf("Freeing\n");
  free(pathParts);
  pathParts = NULL;

  return 0;
}




/****************************************************
*  fs_delete
****************************************************/
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

  //Remove dirEntry from the parent dir
  rmEntry(fileNameToRemove, parentDir);

  //Rewrite parent dir to disk
  writeTableData(parentDir, parentDir->location);

  printf("Freeing\n");
  free(pathParts);
  pathParts = NULL;

  return 0;
}

