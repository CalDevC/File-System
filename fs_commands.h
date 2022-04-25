/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: fs_commands.h
*
* Description: This file holds the prototypes of our helper
* functions which are defined in fs_commands.c along with
* the structure for our volume control block and some global
* variables.
*
**************************************************************/

#ifndef FS_COMMANDS_H
#define FS_COMMANDS_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "fsLow.h"
#include "mfs.h"

#define SIG 90981  //Volume signature
#define FREE_SPACE_START_BLOCK 1
#define NUM_FREE_SPACE_BLOCKS 5
#define DIR_SIZE 5

struct volumeCtrlBlock {
  long signature;      //Marker left behind that can be checked
                       //to know if the disk is setup correctly 
  int blockSize;       //The size of each block in bytes
  long blockCount;	   //The number of blocks in the file system
  long numFreeBlocks;  //The number of blocks not in use
  int rootDir;		     //Block number where root starts
  int freeBlockNum;    //To store the block number where our bitmap starts
} volumeCtrlBlock;

// Pointer to our root directory (hash table of directory entries)
hashTable* workingDir;
int blockSize;
int numOfInts;

//Reads a directory from disk into a hash table (directory) on the heap
hashTable* readTableData(int lbaPosition);

//Writes a hash table (directory) on the heap out to the disk
void writeTableData(hashTable* table, int lbaPosition);

//Checks if the specified path is a directory (1 = yes, 0 =no) but 
//will return -1 if the parent path is invalid 
int isDirWithValidPath(char* path);

//Turns a provided path into a deconstructed path struct (deconPath)
//(Seperates the parent path from the last element in the path)
deconPath* splitPath(char* fullPath);

//Gets the next available block number that is not in use
int getFreeBlockNum(int getNumBlocks);

//Updates the free space bit vector with allocated blocks
void setBlocksAsAllocated(int freeBlock, int blocksAllocated);

//Updates the free space bit vector with freed blocks
void setBlocksAsFree(int freeBlock, int blocksFreed);

//Prints out the details of a directory entry
int fs_stat(const char* path, struct fs_stat* buf);

//Check if a path is a directory (1 = yes, 0 = no)
int fs_isDir(char* path);

// Check if a path is a file (1 = yes, 0 = no)
int fs_isFile(char* path);

// Opens a directory stream corresponding to 'name', and returns
// a pointer to the directory stream
fdDir* fs_opendir(const char* name);

// Closes the directory stream associated with dirp
int fs_closedir(fdDir* dirp);

//Moves to the next entry in the directory associated with dirp and
//returns its info
struct fs_diriteminfo* fs_readdir(fdDir* dirp);

//Returns a pointer to the directory specified by buf if it exists
hashTable* getDir(char* buf);

//Parses the input string on '/' and stores each resulting string
//into an array of strings to return
char** stringParser(char* inputStr);

//Sets the current working directory to the directory specified by 
//buf if it exists
int fs_setcwd(char* buf);

//Gets the current working directory
char* fs_getcwd(char* buf, size_t size);

//Creates a new directory
int fs_mkdir(const char* pathname, mode_t mode);

//Removes a directory if its empty
int fs_rmdir(const char* pathname);

//Removes a file
int fs_delete(char* filename);

#endif