//
// Created by Davide on 29/01/2023.
//
#include "FileTable.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "FtpCommands.h"

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

void RemoveFile(FileTable *ft, char *name) {
    int index = FindFile(ft, name);
    if (index == -1) {
        return;
    }
    ft->size--;
    ft->files[index] = ft->files[ft->size];
    ft->files = realloc(ft->files, sizeof(File) * ft->size);
}

void DestroyFileTable(FileTable *ft) {
    free(ft->files);
    free(ft);
}

// Implementation of the UserTable
