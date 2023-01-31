//
// Created by Davide on 29/01/2023.
//

#ifndef DFTP_FILETABLE_H
#define DFTP_FILETABLE_H

#include <stdio.h>

// Define a file, that has a name, a size, a mod date
typedef struct {
    char *name;
    ssize_t size;
    long moddate;
    void *content;
} File;

// Define a file table, that has a list of files
typedef struct {
    File *files;
    int size;
} TabellaFile;

// Create a new file table
TabellaFile *NewFileTable();

// Add a file to the file table
void AddFile(TabellaFile *file_table, File file);

// Remove a file from the file table
void RemoveFile(TabellaFile *file_table, char *file_name);

// Get a file from the file table
File GetFile(TabellaFile *file_table, char *file_name);

TabellaFile *CreateFileTable();

// Get the list of files from the file table
File *GetFiles(TabellaFile *file_table);

char *GetFilesMlsd(TabellaFile *file_table);

char *GetFilesList(TabellaFile *file_table);

// Get the number of files in the file table
int GetFilesCount(TabellaFile *file_table);

// Free the file table
void FreeFileTable(TabellaFile *file_table);

File CreateFile(char *name, ssize_t size, long moddate, char *content);


int FindFile(TabellaFile *ft, char *name);

void RenameFile(TabellaFile *ft, char *from, char *to);

#endif //DFTP_FILETABLE_H
