//
// Created by Davide on 29/01/2023.
//

#ifndef DFTP_FILETABLE_H
#define DFTP_FILETABLE_H

// Define a file, that has a name, a size, a mod date
typedef struct {
    char *name;
    int size;
    char *mod_date;
} File;

// Define a file table, that has a list of files
typedef struct {
    File *files;
    int size;
} FileTable;

// Create a new file table
FileTable *NewFileTable();

// Add a file to the file table
void AddFile(FileTable *file_table, File file);

// Remove a file from the file table
void RemoveFile(FileTable *file_table, char *file_name);

// Get a file from the file table
File GetFile(FileTable *file_table, char *file_name);
FileTable *CreateFileTable();
// Get the list of files from the file table
File *GetFiles(FileTable *file_table);

char *GetFilesMlsd(FileTable *file_table);

char *GetFilesList(FileTable *file_table);
// Get the number of files in the file table
int GetFilesCount(FileTable *file_table);

// Free the file table
void FreeFileTable(FileTable *file_table);



#endif //DFTP_FILETABLE_H
