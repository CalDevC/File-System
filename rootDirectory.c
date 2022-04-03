#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#define ENTRIES_PER_BLOCK 16

typedef struct {
  int location;//The block number where the start of the file is stored
  char filename[20];       //The name of the file (provided by creator)
  unsigned int fileSize;   //Length of file in bytes
  time_t dateModified;      //Date file was last modified
  time_t dateCreated;	  //Date file was created
} dirEntry;

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

//Hashmap functions
#define SIZE 1000

typedef struct node {
  char key[20];
  dirEntry* value;
  struct node* next;  //Will point to the next object at that map position
} node;

typedef struct hashmap {
  node* entries[SIZE];
} hashmap;

hashmap* hashmapInit() {
  hashmap* newMap = malloc(sizeof(hashmap));

  for (int i = 0; i < SIZE; i++) {
    node* temp = NULL;
    newMap->entries[0] = temp;
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

void setEntry(char key[20], dirEntry* value, hashmap* map) {
  int hashVal = hash(key);
  node* entry = map->entries[hashVal];

  if (entry == NULL) {
    entry = malloc(sizeof(entry));
    strcpy(entry->key, key);
    entry->value = value;
    entry->next = NULL;
    return;
  }

  node* prevEntry;
  while (entry != NULL) {
    if (strcmp(entry->key, key) == 0) {
      free(entry->value);
      entry->value = value;
      return;
    }

    prevEntry = entry;
    entry = prevEntry->next;
  }

  entry = malloc(sizeof(entry));
  strcpy(entry->key, key);
  entry->value = value;
  entry->next = NULL;

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

void clean(hashmap* mapToFree) {
  free(mapToFree);
}

int main() {
  printf("Hash %d\n", hash("filenames"));
  hashmap* testMap = hashmapInit();
  printf("Releasing memory\n");
  clean(testMap);
}