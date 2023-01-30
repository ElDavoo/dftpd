#include "main.h"
//
// Created by Davide on 29/01/2023.
//
// List of tuples (command, function)
// The function must return a string

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <malloc.h>
#include "FtpCommands.h"
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "UserTable.h"
#include "FileTable.h"

Response responses[] = {
        {230, "User logged in, proceed"},
        {331, "User name okay, need password"},
        {530, "Not logged in"},
        {215, "UNIX Type: L8"},
        {211, "End"},
        {257, "\"/\" is current directory"},
        {200, "Command okay"},
        {226, "Closing data connection"},
        {150, "File status okay; about to open data connection"},
        {550, "File unavailable"},
        {250, "Requested file action okay, completed"},
        {425, "Use PASV first."},
        {226, "Closing data connection"},
        {221, "Service closing control connection"},
        {500, "Syntax error, command unrecognized"},
        {501, "Syntax error in parameters or arguments"},
        {502, "Command not implemented"},
        {503, "Bad sequence of commands"},
        {504, "Command not implemented for that parameter"},
        {530, "Not logged in"},
        {550, "Requested action not taken"},
        {551, "Requested action aborted: page type unknown"},
        {552, "Requested file action aborted"},
        {553, "Requested action not taken"}
};

OpenedSocket *data_socket = NULL;

TransferMode transfer_mode = ASCII;

void SendToSocket(int socket, char *command) {
    // Add the \r by creating a new string
    char *new_command = malloc(strlen(command) + 2);
    strcpy(new_command, command);
    strcat(new_command, "\r");
    send(socket, new_command, strlen(new_command), 0);
    free(new_command);
}

void SendEndCommand(int socket, int status, char *msg);

void SendOneLineCommand(int socket, int status){
    // Search in responses[] the status and send it
    for (int i = 0; i < sizeof(responses) / sizeof(responses[0]); i++) {
        if (responses[i].status == status) {
            SendEndCommand(socket, status, responses[i].message);
            return;
        }
    }
    SendOneLineCommand(socket, 502);
}

void SendEndCommand(int socket, int status, char *msg) {// Prepend the status to the message
    char *message = malloc(strlen(msg) + 4);
    sprintf(message, "%d %s", status, msg);
    SendToSocket(socket, message);
    free(message);
}


void SendStartCommand(int socket, int status, char *msg) {// Prepend the status to the message
    char *message = malloc(strlen(msg) + 4);
    sprintf(message, "%d-%s", status, msg);
    SendToSocket(socket, message);
    free(message);
}

// List of commands
void OnUser(int socket, char *username) {

    // CHeck if users_file_path is set
    if (users_file_path[0] == '\0') {
        // Authentication is disabled, accept
        SendOneLineCommand(socket, 230);
        return ;
    }

    // Get the user from the file
    User usr = GetUserFromFile(username, "users.txt");
    if (usr.username == NULL) {
        // Send the error
        SendOneLineCommand(socket, 530);
        return ;
    }
    // Send the success
    SendOneLineCommand(socket, 331);
    return ;

}

void OnSyst(int socket, char *args) {
    SendOneLineCommand(socket, 215);
}
char *features[] = {};

void OnFeat(int socket, char *args) {
    SendStartCommand(socket, 211, "Features:");
    int a = sizeof(features);
    int b = sizeof(features[0]);
    for (int i = 0; i < sizeof(features) / sizeof(features[0]); i++) {
        // Prepend a space and add \r to the string
        char *feature = malloc(strlen(features[i]) + 2);
        strcpy(feature, " ");
        strcat(feature, features[i]);
        // add \r
        strcat(feature, "\r");
        SendToSocket(socket, feature);
        free(feature);
    }
    SendOneLineCommand(socket, 211);
}

void OnPwd(int socket, char *args) {
    SendOneLineCommand(socket, 257);
}

void OnType(int socket, char *args) {
    switch (args[0]) {
        case 'A':
            transfer_mode = ASCII;
            SendOneLineCommand(socket, 200);
            return ;
        case 'I':
            transfer_mode = BINARY;
            SendOneLineCommand(socket, 200);
            return ;
        default:
            SendOneLineCommand(socket, 504);
            return ;
    }
}

void OnPasv(int sk, char *args) {
    // If the data socket is already in the list, remove it
    // If the data socket is already open, close it
    if (data_socket != NULL) {
        RemoveOpenedSocket(openedSockets, data_socket->open_port);
        close(data_socket->socket);
        free(data_socket);
        data_socket = NULL;
    }
    data_socket = malloc(sizeof(OpenedSocket));

    // Take a random port between 50000 and 60000
    int port = 50000 + rand() % 100;
    // Create the socket
    data_socket->socket = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (data_socket->socket == -1) {
        SendOneLineCommand(sk, 500);
        return ;
    }
    // Bind the socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // take the s_addr from sk
    struct sockaddr_in sk_addr;
    socklen_t sk_addr_len = sizeof(sk_addr);
    getsockname(sk, (struct sockaddr *) &sk_addr, &sk_addr_len);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(data_socket->socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind: ");
        SendOneLineCommand(sk, 500);
        return ;
    }
    // Listen
    if (listen(data_socket->socket, 1) == -1) {
        SendOneLineCommand(sk, 500);
        return ;
    }
    // Send the response
    char *response = malloc(100);
    sprintf(response, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
            (int) (sk_addr.sin_addr.s_addr & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 8) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 16) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 24) & 0xFF),
            (int) (port >> 8),
            (int) (port & 0xFF));
    SendEndCommand(sk, 227, response);
    free(response);
    // Add the socket to the list of sockets
    AddOpenedSocket(openedSockets, data_socket);


}

void OnQuit(int socket, char *args) {
    SendOneLineCommand(socket, 221);
    // Disconnect the client
    close(socket);
}

void OnList(int socket, char *args) {
    // Check if the data socket is open
    if (data_socket == NULL) {
        SendOneLineCommand(socket, 425);
        return ;
    }
    // Check if the data socket is listening
    if (data_socket->socket == -1) {
        SendOneLineCommand(socket, 425);
        return ;
    }
    // Accept the connection
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        SendOneLineCommand(socket, 425);
        return ;
    }
    // Send the response
    SendOneLineCommand(socket, 150);
    // Send the list
    char *list = GetFilesList(file_table);
    // Close the data socket
    close(data_sk);
    // Send the response
    SendOneLineCommand(socket, 226);
}

Command cmds [] = {
        {"USER", OnUser},
        {"SYST", OnSyst},
        {"FEAT", OnFeat},
        {"PWD", OnPwd},
        {"TYPE", OnType},
        {"PASV", OnPasv},
        {"QUIT", OnQuit},
        {"LIST", OnList},
};



// Parse the request
Request ParseRequest(char *request) {
    Request req;
    char *token = strtok(request, " ");
    // Clean the request from \r and \n
    for (int i = 0; i < strlen(token); i++) {
        if (token[i] == '\r' || token[i] == '\n') {
            token[i] = '\0';
        }
    }
    req.command = token;
    token = strtok(NULL, " ");
    if (token == NULL) {
        req.args = NULL;
        return req;
    }
    // Clean the request from \r and \n
    for (int i = 0; i < strlen(token); i++) {
        if (token[i] == '\r' || token[i] == '\n') {
            token[i] = '\0';
        }
    }
    req.args = token;
    return req;
}

void HandleRequest(int socket, char *request) {
    bool command_found = false;
    // Parse the request
    Request req = ParseRequest(request);
    // Search in cmds[] the command and execute it
    // We can't use sizeof because it returns the size of the pointer
    for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        if (strcmp(cmds[i].command, req.command) == 0) {
            cmds[i].function(socket, req.args);
            command_found = true;
            break;
        }
    }
    if (!command_found) {
        SendOneLineCommand(socket, 502);
    }
}