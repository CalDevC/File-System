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
  tableData* data = malloc(DIR_SIZE * blockSize);
  if (!data) {
    mallocFailed();
  }

  LBAread(data, DIR_SIZE, lbaPosition);

  dirEntry* arr = data->arr;

  //Create a new hash table to be populated
  hashTable* dirPtr = hashTableInit(data->dirName, ((DIR_SIZE * blockSize) / sizeof(dirEntry) - 1),
    lbaPosition);

  //Loop through all entries in arr and add them to the new hash table
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
  if (!data) {
    mallocFailed();
  }

  dirEntry* arr = calloc(numEntries, 1);
  if (!arr) {
    mallocFailed();
  }

  //Copy the hash table's name to the tableData object so that it
  //can be written to the disk
  strncpy(data->dirName, table->dirName, strlen(table->dirName));

  int j = 0;  //j will track indcies for the array of directory entries

  //Iterate through the whole table to find every directory entry that is in use
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

  memcpy(data->arr, arr, numEntries);

  //Write the array out to the specified block numbers
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

  //Determin if the provide path is absolute or relative
  int absPath = strcmp(parsedPath[0], "/") == 0;

  //Check if path is root or empty
  if (parsedPath[0] == NULL || (absPath && parsedPath[1] == NULL)) {
    //If path is empty return -1 for error otherwise return 1 because it is root
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
  if (absPath) {  //If the path is absolute, start at root
    currDir = getDir("/");
  } else {  //Otherwise start in the current working directory
    currDir = readTableData(workingDir->location);
  }

  int i = 0;

  //If the path is absolute we will skip the first component of the path
  //because we are already starting at root
  if (absPath) {
    i++;
  }

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

  //Error if the last path component does not exist
  if (entry == NULL) {
    free(currDir);
    currDir = NULL;
    return -1;
  }

  //Now that we know the path is valid we can return either 
  //0 if it is a file or 1 if it is a directory
  int result = entry->isDir;

  free(currDir);
  currDir = NULL;

  return result;
}


//Seperate a path into its parent path and child compoent and store it 
//in a deconstructed path (deconPath) struct
deconPath* splitPath(char* fullPath) {
  char** parsedPath = stringParser(fullPath);
  char* parentPath = malloc(strlen(fullPath) + 1);
  if (!parentPath) {
    mallocFailed();
  }

  //Build the parent path by concatenating each of the path components 
  //provided by stringParser (excluding the final one) with a '/' in 
  //between each component and a NULL character at the end
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

  //Add the components to a deconstructed path struct
  deconPath* pathParts = malloc(sizeof(deconPath));
  if (!pathParts) {
    mallocFailed();
  }
  pathParts->parentPath = parentPath;
  pathParts->childName = parsedPath[i];

  //Make sure that parentPath and childName are always initialized with 
  //something. Infer that we are using the current directory if none 
  //is specified
  if (strcmp(pathParts->parentPath, "") == 0) {
    pathParts->parentPath = ".";
  } else if (strcmp(pathParts->childName, "") == 0) {
    pathParts->childName = pathParts->parentPath;
    pathParts->parentPath = ".";
  }

  return pathParts;
}


int getFreeBlockNum(int getNumBlocks) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  // Read the bitvector
  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

  // This will help determine the first block number that is
  // free
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
          free(bitVector);
          bitVector = NULL;
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
  free(bitVector);
  bitVector = NULL;
  return -1;
}


//Updates the free space bit vector with allocated blocks
void setBlocksAsAllocated(int freeBlock, int blocksAllocated) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

  freeBlock += 1;

  int bitNum = freeBlock - ((intBlock * 32) + 32);

  if (bitNum < 0) {
    bitNum *= -1;
  }

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
    bitVector[intBlock] = bitVector[intBlock] & ~(1 << (32 - index));
  }

  LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, 1);
  free(bitVector);
  bitVector = NULL;
}


//Updates the free space bit vector with freed blocks
void setBlocksAsFree(int freeBlock, int blocksFreed) {
  int* bitVector = malloc(NUM_FREE_SPACE_BLOCKS * blockSize);
  if (!bitVector) {
    mallocFailed();
  }

  LBAread(bitVector, NUM_FREE_SPACE_BLOCKS, FREE_SPACE_START_BLOCK);

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

  // We want to start from the bitnum (index) and go up to the bitNum + 
  // blocksAllocated while setting the bits to 1, representing that the
  // corresponding blocks are free
  int total = (bitNum + blocksFreed);

  for (; index < total; index++) {
    // If we have reached the end of an integer, we move on to the next
    // integer, and reset the index
    if (index > 32) {
      intBlock += 1;
      index = 1;
      total -= 32;
    }

    // We first create a bit mask by shifting 1 to the left, to a
    // position we want to set the bit at, calculated using (32 - index)
    // then we apply the mask to our integer, which will 
    // basically set the corresponding bit
    bitVector[intBlock] = bitVector[intBlock] | (1 << (32 - index));
  }

  LBAwrite(bitVector, NUM_FREE_SPACE_BLOCKS, 1);
  free(bitVector);
  bitVector = NULL;
}


//Displays file details associated with the file system
int fs_stat(const char* path, struct fs_stat* buf) {

  if (path == NULL) {
    return -1;
  }

  //Split the parent path from the child component
  deconPath* pathParts = splitPath((char*)path);

  //Get the child's directory entry from its parent directory
  hashTable* dir = getDir(pathParts->parentPath);
  dirEntry* entry = getEntry(pathParts->childName, dir);

  if (entry == NULL) {
    return -1;
  }

  //Display file info
  printf("File: \t%s\n", entry->filename);

  buf->st_size = entry->fileSize;
  printf("Size: \t%ld\n", buf->st_size);

  buf->st_blksize = blockSize;
  printf("IO Block size: \t%ld\n", buf->st_blksize);

  buf->st_blocks = entry->fileSize / blockSize;
  printf("Blocks: \t%ld\n", buf->st_blocks);

  //Get and store current time
  time_t currentTime;
  struct tm ts;
  char time_buf[80];

  time(&currentTime);

  // Store epoch time, but print out formatted time
  buf->st_accesstime = entry->dateModified;
  ts = *localtime(&buf->st_accesstime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Last Accessed: \t%s\n", time_buf);

  buf->st_modtime = entry->dateModified;
  ts = *localtime(&buf->st_modtime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Last Modified: \t%s\n", time_buf);

  buf->st_createtime = entry->dateCreated;
  ts = *localtime(&buf->st_createtime);
  strftime(time_buf, sizeof(time_buf), "%a %Y-%m-%d %H:%M:%S %Z", &ts);
  printf("Created: \t%s\n", time_buf);

  free(pathParts->childName);
  pathParts->childName = NULL;
  free(pathParts);
  pathParts = NULL;
  free(dir);
  dir = NULL;

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

  // Get the directory corresponding to the name
  // passed as an argument
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

  //These static variables are used to track our position in the linked list 
  //if we have more than 1 value hashed to a location
  static int prevIdx = -999;  //The previously located index
  //The number of times we have visited the previous index
  static int prevIdxCount = 0;

  //Update the number of times we have found the previous index
  if (prevIdx == dirEntIdx) {
    prevIdxCount++;
  } else {
    prevIdxCount = 0;
  }

  //Return NULL to signify we have reached the end of the directory
  if (dirEntIdx == dirp->maxIdx) {
    return NULL;
  }

  //Update descriptor with current location in directory
  dirp->dirEntryPosition = dirEntIdx;

  //Get the information of the entry at the found index and use it to 
  //create a dirItemInfo instance to return
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

  // We need to copy the orginial path because the strtok_r function
  // modifies the string that it receives as a parameter, so we
  // cannot pass the original string
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

  // We keep dividing the string into substrings until
  // we get NULL as a substring
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
  // We call splitPath() to split the given path into 
  // child and parent path
  deconPath* pathParts = splitPath((char*)pathname);
  char* parentPath = pathParts->parentPath;

  // We check if the parent path is valid and is a directory, before
  // we create a new directory within it
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
    printf("rm: cannot remove directory '%s': directory does not exist\n", pathname);
    return -1;
  } else if (strcmp(pathname, "/") == 0) {
    printf("rm: cannot remove directory '%s': directory is root\n", pathname);
    return -1;
  }

  hashTable* parentDir = getDir(parentPath);

  int sizeOfEntry = sizeof(dirEntry);	//48 bytes
  int dirSizeInBytes = (DIR_SIZE * blockSize);	//2560 bytes
  int maxNumEntries = (dirSizeInBytes / sizeOfEntry) - 1; //52 entries

  char* dirNameToRemove = pathParts->childName;
  int dirToRemoveLocation = getEntry(dirNameToRemove, parentDir)->location;
  hashTable* dirToRemove = readTableData(dirToRemoveLocation);

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

  //Remove dirEntry from the parent dir
  rmEntry(fileNameToRemove, parentDir);

  //Rewrite parent dir to disk
  writeTableData(parentDir, parentDir->location);

  free(pathParts);
  pathParts = NULL;

  return 0;
}

