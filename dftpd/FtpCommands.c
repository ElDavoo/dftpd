#include "main.h"
//
// Created by Davide on 29/01/2023.
//
// List of tuples (command, function)
// The function must return a string

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "FtpCommands.h"
#include "UserTable.h"

// List of commands
void OnUser(int socket, char *username) {

    // CHeck if users_file_path is set
    if (users_file_path[0] == '\0') {
        // Authentication is disabled, accept
        send(socket, "230 Authentication disabled\r", 28, 0);
        return ;
    }

    // Get the user from the file
    User usr = GetUserFromFile(username, "users.txt");
    if (usr.username == NULL) {
        // Send the error
        send(socket, "530 User not found\r", 19, 0);
        return ;
    }
    // Send the success
    send(socket, "331 User found\r", 15, 0);
    return ;

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