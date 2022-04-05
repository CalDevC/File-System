/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: PatrickCeledio, CalDevC, GurinderS120, jonathanluu0
* Group Name: Sudoers
* Project: Basic File System
*
* File: directory.h
*
* Description: This file defines the structure of our directory
* entry, a function to initialize a directory entry, and the
* implementation of our hashTable which we use to store directory
* entries.
*
**************************************************************/

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#define ENTRIES_PER_BLOCK 16

typedef struct dirEntry {
  int isDir;              //1 if entry is a directory, 0 if it is a file
  int location;           //The block number where the start of the file is stored
  char filename[20];      //The name of the file (provided by creator)
  unsigned int fileSize;  //Length of file in bytes
  time_t dateModified;    //Date file was last modified
  time_t dateCreated;	    //Date file was created
} dirEntry;

//Initialize a new directory entry
dirEntry* dirEntryInit(char filename[20], int isDir, int location,
  unsigned int fileSize, time_t dateModified, time_t dateCreated) {

  dirEntry* entry = malloc(sizeof(dirEntry));

  strcpy(entry->filename, filename);
  entry->isDir = isDir;
  entry->location = location;
  entry->fileSize = fileSize;
  entry->dateModified = dateModified;
  entry->dateCreated = dateCreated;

  return entry;
}

// *************************** Hash Table functions *************************** //
#define SIZE 512  

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
} hashTable;

//Get the hash value for a given key (filenames are used as keys)
int hash(const char filename[20]) {
  int value = 1;

  //Alter the hash value based on each char in 
  //the name to try to get a unique value
  for (int i = 0; i < 20; i++) {
    value *= 2 + filename[i];
  }

  //If the integer overflows, correct the value to be positive
  if (value < 0) {
    value *= -1;
  }

  //Return a value that will definitely be a valid index in the table
  //by using the remainder of (calculated value / table size)
  return value % SIZE;
}

//Initialize an entry for the hash table
node* entryInit(char key[20], dirEntry* value) {
  //allocate memory for the entry in the table and 
  //the entry's value (directory entry)
  node* entry = malloc(sizeof(node));
  entry->value = malloc(sizeof(dirEntry));

  //Transfer the data to the allocated memory
  strcpy(entry->key, key);
  memcpy(entry->value, value, sizeof(dirEntry));
  entry->next = NULL;

  return entry;
}

//Initialize a new hashTable
hashTable* hashTableInit(int maxNumEntries) {
  hashTable* table = malloc(sizeof(node*) * SIZE);
  table->maxNumEntries = maxNumEntries;

  //Each node in the table should be set to a directory entry with a filename
  //of "" so that we know if there is a collision or not
  for (int i = 0; i < SIZE; i++) {
    dirEntry* entryVal = dirEntryInit("", 0, 0, 0, time(NULL), time(NULL));
    node* entry = entryInit("", entryVal);
    table->entries[i] = entry;
  }

  table->numEntries = 0;

  return table;
}

//Update an existing entry or add a new one
void setEntry(char key[20], dirEntry* value, hashTable* table) {
  //Get the entry based on the hash value calculated from the key
  int hashVal = hash(key);
  node* entry = table->entries[hashVal];

  //If we have already reached the maximum capacity of the directory,
  //Don't attempt to create another directory entry
  if (table->numEntries == table->maxNumEntries) {
    printf("Directory is full, no directory entry created.\n");
    return;
  }

  table->numEntries++;

  //If there is no collision then add a new initialized entry
  if (strcmp(entry->value->filename, "") == 0) {
    table->entries[hashVal] = entryInit(key, value);
    return;
  }

  node* prevEntry;

  //If there is a collision
  while (entry != NULL) {

    //If the current entry has the same key that we are attempting 
    //to add then update the existing entry
    if (strcmp(entry->key, key) == 0) {
      free(entry->value);
      entry->value = malloc(sizeof(dirEntry));
      memcpy(entry->value, value, sizeof(dirEntry));
      return;
    }

    //Move on to check the next entry at the current table location
    prevEntry = entry;
    entry = prevEntry->next;
  }

  //If the key was not found at the table location then add it to 
  //the end of the list at that location
  prevEntry->next = entryInit(key, value);

}

//Retrieve an entry from a provided hashTable
dirEntry* getEntry(char key[20], hashTable* table) {
  //Get the entry based on the hash value calculated from the key
  int hashVal = hash(key);
  node* entry = table->entries[hashVal];

  //Checks hashTable for matching key and return its value
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }
  return NULL;
}

//Write out the hash table contents to the console for debug
void printTable(hashTable* table) {
  printf("******** table ********");
  for (int i = 0; i < SIZE; i++) {
    node* entry = table->entries[i];
    if (strcmp(entry->value->filename, "") != 0) {
      printf("\n[Entry %d] %s", i, table->entries[i]->key);
      while (entry->next != NULL) {
        entry = entry->next;
        printf(", %s", entry->key);
      }
    }
  }

}

//Free the memory allocated to the hashTable
void clean(hashTable* table) {
  for (int i = 0; i < SIZE; i++) {
    node* entry = table->entries[i];
    free(entry);
  }
  free(table);
}
