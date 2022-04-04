/**************************************************************
* Class:  CSC-415-02 Fall 2021
* Names: Patrick Celedio, Chase Alexander
* Student IDs: 920457223, 921040156
* GitHub Name: PatrickCeledio, CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: directory.h
*
* Description: This file defines the structure of our directory
* entry, a function to initialize a directory entry, and the
* implementation of our hashmap which we use to store directory
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

// *************************** Hashmap functions *************************** //
#define SIZE 512  

//Node objects are used to populate the hashmap
typedef struct node {
  char key[20];       //filename 
  dirEntry* value;    //directory entry
  struct node* next;  //points to the next directory entry
} node;

//Hashmap object that holds all of the node entries
typedef struct hashmap {
  node* entries[SIZE];
  int numEntries;
  int maxNumEntries;
} hashmap;

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

  //Return a value that will definitely be a valid index in the map
  //by using the remainder of (calculated value / map size)
  return value % SIZE;
}

//Initialize an entry for the hashmap
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

//Initialize a new hashmap
hashmap* hashmapInit(int maxNumEntries) {
  hashmap* map = malloc(sizeof(node*) * SIZE);
  map->maxNumEntries = maxNumEntries;
  //Each node in the map should be set to NULL so that we know if there 
  //is a collision or not
  for (int i = 0; i < SIZE; i++) {
    dirEntry* entryVal = dirEntryInit("", 0, 0, 0, time(NULL), time(NULL));
    node* entry = entryInit("", entryVal);
    map->entries[i] = entry;
  }

  map->numEntries = 0;

  return map;
}

//Update an existing entry or add a new one
void setEntry(char key[20], dirEntry* value, hashmap* map) {
  //Get the entry based on the hash value calculated from the key
  int hashVal = hash(key);
  node* entry = map->entries[hashVal];
  if (map->numEntries == map->maxNumEntries) {
    printf("Directory is full, no directory entry created.\n");
    return;
  }
  map->numEntries++;

  //If there is no collision then add a new initialized entry
  if (strcmp(entry->value->filename, "") == 0) {
    map->entries[hashVal] = entryInit(key, value);
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

    //Move on to check the next entry at the current map location
    prevEntry = entry;
    entry = prevEntry->next;
  }

  //If the key was not found at the map location then add it to 
  //the end of the list at that location
  prevEntry->next = entryInit(key, value);

}

//Retrieve an entry from a provided hashmap
dirEntry* getEntry(char key[20], hashmap* map) {
  //Get the entry based on the hash value calculated from the key
  int hashVal = hash(key);
  node* entry = map->entries[hashVal];

  //Checks hashmap for matching key and return its value
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }
  return NULL;
}

void printMap(hashmap* map) {
  printf("******** MAP ********");
  for (int i = 0; i < SIZE; i++) {
    node* entry = map->entries[i];
    if (strcmp(entry->value->filename, "") != 0) {
      printf("\n[Entry %d] %s", i, map->entries[i]->key);
      while (entry->next != NULL) {
        entry = entry->next;
        printf(", %s", entry->key);
      }
    }
  }

}

//Write all directory entries in the hashmap to the disk
void writeMapData(hashmap* map, int lbaCount, int lbaPosition) {
  //Create an array whose size is the number of directory entries in map
  int arrSize = map->numEntries;
  dirEntry arr[arrSize];

  //j will track indcies for the array
  int j = 0;

  //iterate through the whole map
  for (int i = 0; i < SIZE; i++) {
    node* entry = map->entries[i];
    if (strcmp(entry->value->filename, "") != 0) {
      arr[j] = *entry->value;  //add entry
      j++;

      //add other entries that are at the same hash location
      while (entry->next != NULL) {
        entry = entry->next;
        arr[j] = *entry->value;  //add entry
        j++;
      }
    }

    //Don't bother lookng through rest of map if all entries are found
    if (j == arrSize - 1) {
      break;
    }
  }

  //Write the array to the disk
  LBAwrite(arr, lbaCount, lbaPosition);
}

//Free the memory allocated to the hashmap
void clean(hashmap* map) {
  for (int i = 0; i < SIZE; i++) {
    node* entry = map->entries[i];
    free(entry);
  }
  free(map);
}
