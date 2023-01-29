//
// Created by Davide on 29/01/2023.
//

#ifndef DFTP_FTPCOMMANDS_H
#define DFTP_FTPCOMMANDS_H


typedef struct {
    char *command;
    char *(*function)(int socket, char *args);
} Command;

typedef struct {
    char *command;
    char *args;
} Request;

Request ParseRequest(char *request);

char * OnUser(int socket, char *args);

void HandleRequest(int socket, char *request);


#endif //DFTP_FTPCOMMANDS_H
