#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#include "directory.h"
#include "mfs.h"
int fs_mkdir(const char *pathname, mode_t mode){
    //validate if directory
    //check if each pathname[] index 
    //if pathname exists in directory, return -1
    //else hashmap* newDir = hashmapInit();
    char* dir[100];

    for(int i = 0; i < dir.length; i++){
        if(strcmp(dir[i], pathname) == 0){
            return -1;
        }
    }
    hashmap* newDir = hashmapInit();

}