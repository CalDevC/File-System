/**************************************************************
* Class:  CSC-415-02 Spring 2022
* Names: Patrick Celedio, Chase Alexander, Gurinder Singh, Jonathan Luu
* Student IDs: 920457223, 921040156, 921369355, 918548844
* GitHub Name: csc415-filesystem-CalDevC
* Group Name: Sudoers
* Project: Basic File System
*
* File: b_io.h
*
* Description: Interface of basic I/O functions
*
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>
#include "fs_commands.h"

typedef int b_io_fd;

// Function to open the specified file for read/write operations
// based on the flags specified, it returns a file descriptor
// that gets used in other file functions as an identifier for 
// a file
b_io_fd b_open(char* filename, int flags);
int b_read(b_io_fd fd, char* buffer, int count);
int b_write(b_io_fd fd, char* buffer, int count);
// Function to change the offset of a file
int b_seek(b_io_fd fd, off_t offset, int whence);
void b_close(b_io_fd fd);

#endif

