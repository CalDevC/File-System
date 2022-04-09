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
    //check for unique pathname
    //if pathname exists in directory, return error
    //else create new hashmap directory

    char* dir[100]; //test parsed pathname array

    //returns an error if directory names match
    for(int i = 0; i < dir.length; i++){
        if(strcmp(dir[i], pathname) == 0){
            return -1;
        }
        //validate if each dir[i] = 1
    }
    //create new directory
    dirEntryInit(pathname, 1, dirSize, time(0), time(0));

    

}