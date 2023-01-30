//
// Created by Davide on 29/01/2023.
//
#include "FileTable.h"
#include <string.h>
#include <stdlib.h>


// Implementation of the FileTable
FileTable *CreateFileTable() {
    FileTable *ft = malloc(sizeof(FileTable));
    ft->size = 0;
    ft->files = malloc(sizeof(File) * ft->size);
    return ft;
}

void AddFile(FileTable *ft, File file) {
    ft->size++;
    ft->files = realloc(ft->files, sizeof(File) * ft->size);
    ft->files[ft->size - 1] = file;
}

int FindFile(FileTable *ft, char *name) {
    for (int i = 0; i < ft->size; i++) {
        if (strcmp(ft->files[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Get a file from the file table
File GetFile(FileTable *file_table, char *file_name) {
    int index = FindFile(file_table, file_name);
    if (index == -1) {
        File file;
        file.name = NULL;
        file.size = 0;
        return file;
    }
    return file_table->files[index];
}

// Get the list of files from the file table
File *GetFiles(FileTable *file_table) {
    return file_table->files;
}

char *GetFilesList(FileTable *file_table) {
    char *files_list = malloc(1);
    files_list[0] = '\0';
    for (int i = 0; i < file_table->size; i++) {
        // Example strings
        // type=file;size=3;modify=20230130164000.170;perms=awr; c
        //type=file;size=4;modify=20230130170447.320;perms=awr; CIAO.txt
        //type=file;size=0;modify=20230130094443.237;perms=awr; CIAO.txt.bak

        char file_info[100];
        sprintf(file_info, "type=file;size=%ld;modify=%ld;perms=awr; %s\r\n", file_table->files[i].size,
                file_table->files[i].moddate, file_table->files[i].name);
        files_list = realloc(files_list, sizeof(char) * (strlen(files_list) + strlen(file_info) + 1));
        strncat(files_list, file_info, strlen(file_info));

    }
    return files_list;
}

char *GetFilesMlsd(FileTable *file_table) {
    char *files_list = malloc(sizeof(char) * 1);
    files_list[0] = '\0';
    for (int i = 0; i < file_table->size; i++) {
        files_list = realloc(files_list, sizeof(char) * (strlen(files_list) + strlen(file_table->files[i].name) + 2));
        strcat(files_list, file_table->files[i].name);
        strcat(files_list, "");
    }
    return files_list;
}

void RemoveFile(FileTable *ft, char *name) {
    int index = FindFile(ft, name);
    if (index == -1) {
        return;
    }
    ft->size--;
    free(ft->files[index].name);
    free(ft->files[index].content);
    ft->files[index] = ft->files[ft->size];
    ft->files = realloc(ft->files, sizeof(File) * ft->size);
}

void DestroyFileTable(FileTable *ft) {
    free(ft->files);
    free(ft);
}

File CreateFile(char *name, ssize_t size, long moddate, char *content) {
    File file;
    // Copy the name into a new pointer
    file.name = malloc(sizeof(char) * (strlen(name) + 1));
    strncpy(file.name, name, strlen(name));
    file.name[strlen(name)] = '\0';
    file.size = size;
    file.moddate = moddate;
    // Copy the content into a new pointer
    file.content = malloc(sizeof(char) * (strlen(content) + 1));
    strncpy(file.content, content, strlen(content));
    return file;
}

// Implementation of the UserTable
