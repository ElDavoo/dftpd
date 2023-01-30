//
// Created by Davide on 29/01/2023.
//

#ifndef DFTP_FTPCOMMANDS_H
#define DFTP_FTPCOMMANDS_H


#include "UserTable.h"

typedef struct {
    char *command;
    void (*function)(int socket, char *args);
} Command;

typedef struct {
    char *command;
    char *args;
} Request;

typedef struct {
    int status;
    char *message;
} Response;

// typedef enum for ascii and binary
typedef enum {
    ASCII,
    BINARY
} TransferMode;

Request ParseRequest(char *request);

void OnUser(int socket, char *username);

void HandleRequest(int socket, char *request);


#endif //DFTP_FTPCOMMANDS_H
