/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: directory.h
*
* Description: This file defines the structures of our directory
* entry, hash table, and nodes within our hash table. It also
* defines our function to initialize a directory entry, and the
* functions related to our hashTable.
*
**************************************************************/
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"

#define ENTRIES_PER_BLOCK 16
#define SIZE 53  

typedef struct dirEntry {
  int isDir;              //1 if entry is a directory, 0 if it is a file
  int location;           //The block number where the start of the file is stored
  char filename[20];      //The name of the file (provided by creator)
  unsigned int fileSize;  //Length of file in bytes
  time_t dateModified;    //Date file was last modified
  time_t dateCreated;	    //Date file was created
} dirEntry;

//Node objects are used to populate the hash table
typedef struct node {
  char key[20];       //filename 
  dirEntry* value;    //directory entry
  struct node* next;  //points to the next directory entry
} node;

//Hash Table object that holds all of the node entries
typedef struct hashTable {
  node* entries[SIZE];
  int numEntries;
  int maxNumEntries;
  int location;
} hashTable;

//Initialize a new directory entry
dirEntry* dirEntryInit(char filename[20], int isDir, int location,
  unsigned int fileSize, time_t dateModified, time_t dateCreated);

//Get the hash value for a given key (filenames are used as keys)
int hash(const char filename[20]);

//Initialize an entry for the hash table
node* entryInit(char key[20], dirEntry* value);

//Initialize a new hashTable
hashTable* hashTableInit(int maxNumEntries, int location);

//Update an existing entry or add a new one
void setEntry(char key[20], dirEntry* value, hashTable* table);

//Retrieve an entry from a provided hashTable
dirEntry* getEntry(char key[20], hashTable* table);

//Given an index, find the index of the next entry in the table
int getNextIdx(int currIdx, hashTable* table);

//Write out the hash table contents to the console for debug
void printTable(hashTable* table);

//Free the memory allocated to the hashTable
void clean(hashTable* table);

#endif