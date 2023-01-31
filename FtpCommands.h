//
// Created by Davide on 29/01/2023.
//

#ifndef DFTP_FTPCOMMANDS_H
#define DFTP_FTPCOMMANDS_H


#include "UserTable.h"
#include <pthread.h>

typedef struct {
    char *command;

    void (*function)(int socket, OpenedSocket *data_socket, char *args);
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

void OnUser(int socket, OpenedSocket *data_socket, char *username);

void HandleRequest(int socket, OpenedSocket *data_socket, char *request);

extern pthread_mutex_t lock;
void SendOneLineCommand(int socket, int status);

#endif //DFTP_FTPCOMMANDS_H
