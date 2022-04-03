#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#define ENTRIES_PER_BLOCK 16

typedef struct dirEntry {
  int location;           //The block number where the start of the file is stored
  char filename[20];      //The name of the file (provided by creator)
  unsigned int fileSize;  //Length of file in bytes
  time_t dateModified;    //Date file was last modified
  time_t dateCreated;	    //Date file was created
} dirEntry;

//Initialize a new directory entry
dirEntry* dirEntryInit(char filename[20], int location, unsigned int fileSize,
  time_t dateModified, time_t dateCreated) {

  dirEntry* entry = malloc(sizeof(dirEntry));

  strcpy(entry->filename, filename);
  entry->location = location;
  entry->fileSize = fileSize;
  entry->dateModified = dateModified;
  entry->dateCreated = dateCreated;

  return entry;
}

// *************************** Hashmap functions *************************** //
#define SIZE 1000  //Number of positions in the hashmap

//Node objects are used to populate the hasmap
typedef struct node {
  char key[20];       //The filename will be used as the key
  dirEntry* value;    //The directory entry associated with the filename
  struct node* next;  //points to the next object at that map position
} node;

//Hashmap object that holds all of the node entries
typedef struct hashmap {
  node* entries[SIZE];
} hashmap;

//Initialize a new hashmap
hashmap* hashmapInit() {
  hashmap* newMap = malloc(sizeof(node*) * SIZE);

  for (int i = 0; i < SIZE; i++) {
    node* temp = NULL;
    newMap->entries[i] = temp;
  }

  return newMap;
}

int hash(const char filename[20]) {
  int value = 1;
  for (int i = 0; i < 20; i++) {
    // printf("Char value is %d making value %d\n", filename[i], value);
    value *= 2 + filename[i];
  }

  if (value < 0) {
    value *= -1;
  }

  // printf("Total is %d\n", value);
  return value % SIZE;
}

node* entryInit(char key[20], dirEntry* value) {
  node* entry = malloc(sizeof(node));
  entry->value = malloc(sizeof(dirEntry));
  strcpy(entry->key, key);
  memcpy(entry->value, value, sizeof(dirEntry));
  entry->next = NULL;
  return entry;
}

void setEntry(char key[20], dirEntry* value, hashmap* map) {
  int hashVal = hash(key);
  node* entry = map->entries[hashVal];

  if (entry == NULL) {
    map->entries[hashVal] = entryInit(key, value);
    return;
  }

  node* prevEntry;
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      free(entry->value);
      entry->value = malloc(sizeof(dirEntry));
      memcpy(entry->value, value, sizeof(dirEntry));
      return;
    }

    prevEntry = entry;
    entry = prevEntry->next;
  }

  prevEntry->next = entryInit(key, value);

}

dirEntry* getEntry(char key[20], hashmap* map) {
  int hashVal = hash(key);
  node* entry = map->entries[hashVal];

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
    if (entry != NULL) {
      printf("\n[Entry %d] %s", i, map->entries[i]->key);
      while (entry->next != NULL) {
        entry = entry->next;
        printf(", %s", entry->key);
      }
    }

  }

}

void clean(hashmap* map) {
  for (int i = 0; i < SIZE; i++) {
    node* entry = map->entries[i];
    free(entry);
  }
  free(map);
}

int main() {
  hashmap* map = hashmapInit();
  time_t testTime = time(NULL);

  dirEntry* entry1 = dirEntryInit("myFile", 12, 10, testTime, testTime);
  setEntry("myFile", entry1, map);

  dirEntry* entry2 = dirEntryInit("myFileTest", 12, 10, testTime, testTime);
  setEntry(entry2->filename, entry2, map);

  dirEntry* entry3 = dirEntryInit("myOtherFile", 12, 10, testTime, testTime);
  setEntry(entry3->filename, entry3, map);

  printMap(map);
  printf("\n\nReleasing memory\n");
  clean(map);
}