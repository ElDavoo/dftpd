//
// Created by Davide on 29/01/2023.
//
// List of tuples (command, function)
// The function must return a string

#include <string.h>
#include <unistd.h>
#include "FtpCommands.h"

// List of commands
char * OnUser(int socket, char *args) {
    // send the response
    char *response = "331 User name okay, need password.\r\n";
    write(socket, response, strlen(response));
}

Command cmds [] = {
        {"USER", OnUser},
};

// Parse the request
Request ParseRequest(char *request) {
    Request req;
    char *token = strtok(request, " ");
    req.command = token;
    token = strtok(NULL, " ");
    req.args = token;
    return req;
}

void HandleRequest(int socket, char *request) {
    // Parse the request
    Request req = ParseRequest(request);
    // Search in cmds[] the command and execute it
    // We can't use sizeof because it returns the size of the pointer
    for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        if (strcmp(cmds[i].command, req.command) == 0) {
            cmds[i].function(socket, req.args);
            break;
        }
    }
}