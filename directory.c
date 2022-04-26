/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: directory.c
*
* Description: This file holds the implementations of our
* function to initialize a directory entry and the functions
* related to our hash table.
*
**************************************************************/

#include "directory.h"

//If malloc fails, inform the user and quit
void mallocFailed() {
  printf("Call to malloc memory failed, exiting program...\n");
  exit(-1);
}

//Initialize a new directory entry
dirEntry* dirEntryInit(char filename[20], int isDir, int location,
  unsigned int fileSize, time_t dateModified, time_t dateCreated) {

  dirEntry* entry = malloc(sizeof(dirEntry));
  if (!entry) {
    mallocFailed();
  }

  strcpy(entry->filename, filename);
  entry->isDir = isDir;
  entry->location = location;
  entry->fileSize = fileSize;
  entry->dateModified = dateModified;
  entry->dateCreated = dateCreated;

  return entry;
}

//Get the hash value for a given key (filenames are used as keys)
int hash(const char filename[20]) {
  int value = 1;

  //Alter the hash value based on each char in 
  //the name to try to get a unique value
  int length = strlen(filename);

  for (int i = 0; i < length; i++) {
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
  //allocate memory for the entry in the table (node) and 
  //the entry's value (directory entry)
  node* entry = malloc(sizeof(node));
  if (!entry) {
    mallocFailed();
  }

  entry->value = malloc(sizeof(dirEntry));
  if (!entry->value) {
    mallocFailed();
  }

  //Transfer the data to the allocated memory
  strcpy(entry->key, key);
  memcpy(entry->value, value, sizeof(dirEntry));
  entry->next = NULL;

  return entry;
}

//Initialize a new hashTable
hashTable* hashTableInit(char* dirName, int maxNumEntries, int location) {
  hashTable* table = malloc(sizeof(node) * SIZE);
  if (!table) {
    mallocFailed();
  }

  table->maxNumEntries = maxNumEntries;
  table->location = location;
  strcpy(table->dirName, dirName);

  //Each node in the table should be set to a directory entry with a filename
  //of "" so that we know if there is a collision or not
  for (int i = 0; i < SIZE; i++) {
    dirEntry* entryVal = dirEntryInit("", 0, 0, 0, time(NULL), time(NULL));
    node* entry = entryInit("", entryVal);
    table->entries[i] = entry;
  }

  table->numEntries = 0; //The hash table will start out as empty

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
    printf("Error: cannot create file/directory: directory is full\n");
    return;
  }

  //If there is no collision then add a new initialized entry
  if (strcmp(entry->value->filename, "") == 0) {
    free(entry->value);
    entry->value = NULL;
    free(entry);
    entry = NULL;

    table->entries[hashVal] = entryInit(key, value);
    table->numEntries++;
    return;
  }

  node* prevEntry;

  //If there is a collision
  while (entry != NULL) {

    //If the current entry has the same key that we are attempting 
    //to add then update the existing entry
    if (strcmp(entry->key, key) == 0) {
      entry->value = realloc(entry->value, sizeof(dirEntry));
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
  table->numEntries++;

}

//Retrieve an entry from a provided hashTable
dirEntry* getEntry(char key[20], hashTable* table) {
  //Get the entry based on the hash value calculated from the key
  int hashVal = hash(key);
  node* entry = table->entries[hashVal];

  //Check the linked list at that hashed location for a matching key 
  //and return its value
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      return entry->value;
    }
    entry = entry->next;
  }
<<<<<<< HEAD
  printf("ENTRY IS NULL!\n");

  return NULL;
=======

  return NULL; //Entry not found
>>>>>>> c9a2562f33cfdb71c573f0cb9e602b58edab9440
}

//Remove an existing entry (1 = success, 0 = failed)
int rmEntry(char key[20], hashTable* table) {
  //Get the entry based on the hash value calculated from the key
  int hashVal = hash(key);
  node* entry = table->entries[hashVal];
  node* prevEntry = NULL;

  //If there is a collision
  while (entry != NULL) {

    //If entry was found
    if (strcmp(entry->key, key) == 0) {

      //If the entry is the first in the hashed location
      if (prevEntry == NULL) {
        //If the entry is the only entry at the hashed location
        if (entry->next == NULL) {
          //Set the hashed location as free
          dirEntry* entryVal = dirEntryInit("", 0, 0, 0, time(NULL), time(NULL));
          node* entry = entryInit("", entryVal);
          table->entries[hashVal] = entry;
        } else { //Else there are other entries at the hashed location
          //Set the hashed location's value to the next entry in line
          table->entries[hashVal] = entry->next;
        }

      } else { //Else the entry was not the first at the location
        //Set the previous entry's next to point the same location
        //as the removed entry's next
        prevEntry->next = entry->next;
      }

      table->numEntries--;
      free(entry->value);
      free(entry);
      return 1;
    }

    //Move on to check the next entry at the current table location
    prevEntry = entry;
    entry = prevEntry->next;
  }

  return 0;  //The key was not found
}

//Given an index, find the index of the next entry in the table
int getNextIdx(int currIdx, hashTable* table) {
  int max = table->maxNumEntries;

  //These atatic variables are used to track our position in the linked list 
  //if we have more than 1 value hashed to a location
  static int prevIdx = -999;    //The previously located index
  //The number of times we have returned the previous index
  static int prevIdxCount = 0;

  //Update the number of times we have found the previous index
  if (prevIdx == currIdx) {
    prevIdxCount++;
  } else {
    prevIdxCount = 0;
  }

  //NOTE: fs_openDir will pass max as the initial index
  //If we are starting from the beginning
  if (currIdx == max && strcmp(table->entries[0]->key, "") != 0) {
    //return 0 if it is in use
    return 0;
  } else if (currIdx == max) { //Otherwise start at index 0 and begin checking
    currIdx = 0;
  }

  node* currNode = table->entries[currIdx];

  //Check the next value of the last found index
  for (int i = prevIdxCount; i > 0; i--) {
    currNode = table->entries[currIdx]->next;
  }

  //Return the same index if there is another element hashed to that location
  if (currNode->next != NULL) {
    prevIdx = currIdx;
    return currIdx;
  }

  //Otherwise continue through the table looking for the next non-free entry
  for (int i = currIdx + 1; i < SIZE; i++) {
    currNode = table->entries[i];
    if (strcmp(currNode->value->filename, "") != 0) {
      prevIdx = currIdx;
      return i;
    }
  }

  prevIdx = currIdx;  //Update previous index

  //Return max to signify that we have reached the end of 
  //the entries in the directory
  return max;
}

//Write out the hash table contents to the console for viewing
void printTable(hashTable* table) {
  printf("\n******** %s ********\n", table->dirName); //header

  for (int i = 0; i < SIZE; i++) {
    node* entry = table->entries[i];

    //If the current entry is in use
    if (strcmp(entry->value->filename, "") != 0) {
      //Print the entry's name
      printf("[Entry %d] %s", i, table->entries[i]->key);

      //Print any additional entries hashed to that location
      while (entry->next != NULL) {
        entry = entry->next;
        printf(", %s", entry->key);
      }

      printf("\n");
    }
  }

}

//Free the memory allocated to the hashTable
void clean(hashTable* table) {
  //Store a reference to next so we can access it 
  //after freeing the current entry
  node* next = malloc(sizeof(node));

  //Iterate through the hash table and free every entry and entry->value
  for (int i = 0; i < SIZE; i++) {
    node* entry = table->entries[i];
    if (entry->next) {
      memcpy(next, entry->next, sizeof(node));
    }

    free(entry->value);
    free(entry);

    while (next != NULL) {
      entry = next;
      if (entry->next) {
        memcpy(next, entry->next, sizeof(node));
      } else {
        break;
      }
      free(entry->value);
      free(entry);
    }
  }

  //Free our next pointer and the table
  free(next);
  next = NULL;
  free(table);
  table = NULL;
}