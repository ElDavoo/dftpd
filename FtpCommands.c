#include "main.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <malloc.h>
#include "FtpCommands.h"
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>

#include "FileTable.h"

Response responses[] = {
        {230, "User logged in, proceed"},
        {215, "UNIX Type: L8"},
        {211, "End"},
        {257, "\"/\" is current directory"},
        {200, "Command okay"},
        {350, "OK to rename file"},
        {226, "Closing data connection"},
        {150, "File status okay; about to open data connection"},
        {550, "File unavailable"},
        {250, "Requested file action okay, completed"},
        {425, "Use PASV first."},
        {226, "Closing data connection"},
        {221, "Service closing control connection"},
        {500, "Syntax error, command unrecognized"},
        {502, "Command not implemented"},
        {220, "Welcome to dftpd!"}
};

char *file_to_rename = NULL;

TransferMode transfer_mode = ASCII;
pthread_mutex_t lock;

void SendToSocket(int socket, char *command) {
    size_t oldlength = strlen(command);
    // Add the \r by creating a new string
    char *new_command = malloc(oldlength + 2);
    strncpy(new_command, command, oldlength);
    new_command[oldlength + 1] = '\0';
    new_command[oldlength] = '\r';
    send(socket, new_command, oldlength + 1, 0);
    free(new_command);
}

void SendEndCommand(int socket, int status, char *msg);

void SendOneLineCommand(int socket, int status) {
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
    char *message = malloc(strlen(msg) + 5);
    sprintf(message, "%d %s", status, msg);
    SendToSocket(socket, message);
    free(message);
}


void SendStartCommand(int socket, int status, char *msg) {// Prepend the status to the message
    char *message = malloc(strlen(msg) + 5);
    sprintf(message, "%d-%s", status, msg);
    SendToSocket(socket, message);
    free(message);
}

// List of commands
void OnUser(int socket, OpenedSocket *data_socket, char *username) {
    SendOneLineCommand(socket, 230);
}

void OnSyst(int socket, OpenedSocket *data_socket, char *args) {
    SendOneLineCommand(socket, 215);
}

char *features[] = {};

void OnFeat(int socket, OpenedSocket *data_socket, char *args) {
    SendStartCommand(socket, 211, "Features:");
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

void OnPwd(int socket, OpenedSocket *data_socket, char *args) {
    SendOneLineCommand(socket, 257);
}

void OnType(int socket, OpenedSocket *data_socket, char *args) {
    SendOneLineCommand(socket, 200);
}

void OnPasv(int sk, OpenedSocket *data_socket, char *args) {
    //printf("Before OnPasv\n");
    //PrintOpenedSockets(openedSockets);
    // If the data socket is already in the list, remove it
    // If the data socket is already open, close it
    pthread_mutex_lock(&lock);
    if (data_socket->socket != -1 || data_socket->open_port != 0) {
        RemoveOpenedSocket(socketAperti, data_socket->open_port);
        close(data_socket->socket);
        //free(data_socket);
        //data_socket = NULL;
    }
    //data_socket = malloc(sizeof(OpenedSocket));
    data_socket->open_port = 0;
    data_socket->socket = -1;


    // Create the socket
    data_socket->socket = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (data_socket->socket == -1) {
        SendOneLineCommand(sk, 500);
        perror("socket: ");
        shutdown(data_socket->socket, SHUT_RDWR);
        close(data_socket->socket);
        data_socket->socket = -1;
        data_socket->open_port = 0;
        pthread_mutex_unlock(&lock);
        return;
    }
    // Bind the socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    // Take a random port between 50000 and 60000
    int port = 50010 + rand() % 5;
    addr.sin_port = htons(port);
    // take the s_addr from sk
    struct sockaddr_in sk_addr;
    socklen_t sk_addr_len = sizeof(sk_addr);
    getsockname(sk, (struct sockaddr *) &sk_addr, &sk_addr_len);
    addr.sin_addr.s_addr = INADDR_ANY;
    while (bind(data_socket->socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind: ");
        // Take a random port between 50000 and 60000
        port = 50010 + rand() % 5;
        addr.sin_port = htons(port);
        data_socket->open_port = port;
    }
    // Listen
    if (listen(data_socket->socket, 1) == -1) {
        SendOneLineCommand(sk, 500);
        shutdown(data_socket->socket, SHUT_RDWR);
        close(data_socket->socket);
        data_socket->socket = -1;
        data_socket->open_port = 0;
        return;
    }
    printf("Thread %lu, PASV: Listening on port %d\n", pthread_self() % 100, port);

    // Send the response
    char *response = malloc(100);
    sprintf(response, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
            (int) (sk_addr.sin_addr.s_addr & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 8) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 16) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 24) & 0xFF),
            (int) (port >> 8),
            (int) (port & 0xFF));
    SendEndCommand(sk, 227, response);
    free(response);
    // Add the socket to the list of sockets
    AddOpenedSocket(socketAperti, data_socket);
    //printf("After OnPasv\n");
    //PrintOpenedSockets(openedSockets);
    pthread_mutex_unlock(&lock);

}

void OnQuit(int socket, OpenedSocket *data_socket, char *args) {
    SendOneLineCommand(socket, 221);
    // Disconnect the client
    close(socket);
}

void OnList(int socket, OpenedSocket *data_socket, char *args) {
    //printf("Before OnList\n");
    //PrintOpenedSockets(openedSockets);
    // Check if the data socket is open
    if (data_socket == NULL) {
        perror("data_socket == NULL");
        SendOneLineCommand(socket, 425);
        return;
    }
    // Check if the data socket is listening
    if (data_socket->socket == -1) {
        perror("data_socket->socket == -1");
        SendOneLineCommand(socket, 425);
        return;
    }
    // Accept the connection with timeout
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(data_socket->socket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    // Send the response
    SendOneLineCommand(socket, 150);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        SendOneLineCommand(socket, 425);
        return;
    }

    // Send the list
    char *list = GetFilesList(tabellaFile);
    send(data_sk, list, strlen(list), 0);
    free(list);
    // Close the data socket
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);
    shutdown(data_socket->socket, SHUT_RDWR);
    close(data_socket->socket);
    // Remove the data socket from the list
    RemoveOpenedSocket(socketAperti, data_socket->open_port);
    data_socket->socket = -1;
    data_socket->open_port = 0;
    // Send the response
    SendOneLineCommand(socket, 226);
    //printf("After OnList\n");
    //PrintOpenedSockets(openedSockets);
}

void OnCwd(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the root directory is asked
    if (strcmp(args, "/") == 0) {
        SendOneLineCommand(socket, 250);
        return;
    }
    SendOneLineCommand(socket, 502);
}

void OnStor(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the data socket is open
    if (data_socket == NULL) {
        SendOneLineCommand(socket, 425);
        return;
    }
    // Check if the data socket is listening
    if (data_socket->socket == -1) {
        SendOneLineCommand(socket, 425);
        return;
    }
    // Accept the connection
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        SendOneLineCommand(socket, 425);
        return;
    }
    // Send the response
    SendOneLineCommand(socket, 150);
    // Receive the file here and save it into a string without calling functions
    char *file = malloc(1000000);
    // Fill with FF
    memset(file, 0xFF, 1000000);
    ssize_t file_size = 0;
    ssize_t bytes_read = 0;
    while ((bytes_read = recv(data_sk, file + file_size, 1000000 - file_size, 0)) > 0) {
        file_size += bytes_read;
    }
    file = realloc(file, file_size + 1);
    file[file_size] = '\0';
    printf("Thread %lu, STOR: Received %zd bytes\n", pthread_self() % 100, file_size);
    // printf("Thread %d, STOR: File content: %s\n", pthread_self() % 100, file);
    // Get the current time as YYYYMMDDHHMMSS.sss
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    strftime(s, sizeof(s), "%Y%m%d%H%M%S", tm);
    // as int
    long time = atol(s);

    RemoveFile(tabellaFile, args);
    // Save the file into the file table
    File file_done = CreateFile(args, file_size, time, file);
    AddFile(tabellaFile, file_done);

    // Close the data socket
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);
    shutdown(data_socket->socket, SHUT_RDWR);
    close(data_socket->socket);
    // Remove the data socket from the list
    RemoveOpenedSocket(socketAperti, data_socket->open_port);
    data_socket->socket = -1;
    data_socket->open_port = 0;
    // Send the response
    SendOneLineCommand(socket, 226);
    free(file);
}

void OnRetr(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the data socket is open
    if (data_socket == NULL) {
        SendOneLineCommand(socket, 425);
        return;
    }
    // Check if the data socket is listening
    if (data_socket->socket == -1) {
        SendOneLineCommand(socket, 425);
        return;
    }
    // Accept the connection
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        SendOneLineCommand(socket, 425);
        return;
    }
    // Send the response
    SendOneLineCommand(socket, 150);
    // Get the file from the file table
    File file = GetFile(tabellaFile, args);
    // Send the file
    send(data_sk, file.content, file.size, 0);
    // Close the data socket
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);
    shutdown(data_socket->socket, SHUT_RDWR);
    close(data_socket->socket);
    // Remove the data socket from the list
    RemoveOpenedSocket(socketAperti, data_socket->open_port);
    data_socket->socket = -1;
    data_socket->open_port = 0;
    // Send the response
    SendOneLineCommand(socket, 226);
}

void OnDele(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the file exists
    if (FindFile(tabellaFile, args) == -1) {
        SendOneLineCommand(socket, 550);
        return;
    }
    // Remove the file from the file table
    RemoveFile(tabellaFile, args);
    SendOneLineCommand(socket, 250);
}

void OnRnfr(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the file exists
    if (FindFile(tabellaFile, args) == -1) {
        SendOneLineCommand(socket, 550);
        return;
    }
    // Save the file name
    file_to_rename = calloc(strlen(args) + 1, sizeof(char));
    strncpy(file_to_rename, args, strlen(args));
    SendOneLineCommand(socket, 350);
}

void OnRnto(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the file exists
    if (FindFile(tabellaFile, file_to_rename) == -1) {
        SendOneLineCommand(socket, 550);
        return;
    }
    // Rename the file
    RenameFile(tabellaFile, file_to_rename, args);
    free(file_to_rename);
    file_to_rename = NULL;
    SendOneLineCommand(socket, 250);
}


Command cmds[] = {
        {"USER", OnUser},
        {"SYST", OnSyst},
        {"FEAT", OnFeat},
        {"PWD",  OnPwd},
        {"TYPE", OnType},
        {"PASV", OnPasv},
        {"QUIT", OnQuit},
        {"LIST", OnList},
        {"CWD",  OnCwd},
        {"STOR", OnStor},
        {"RETR", OnRetr},
        {"DELE", OnDele},
        {"RNFR", OnRnfr},
        {"RNTO", OnRnto},
};


// Parse the request
Request ParseRequest(char *request) {
    Request req = {NULL, NULL};
    char *token = strtok(request, " ");
    // Clean the request from \r and \n
    for (int i = 0; i < strlen(token); i++) {
        if (token[i] == '\r' || token[i] == '\n') {
            token[i] = '\0';
        }
    }
    req.command = calloc(strlen(token) + 1, sizeof(char));
    strncpy(req.command, token, strlen(token));
    // Don't use strtok, but get the rest of the string
    token = request + strlen(token) + 1;
    // Clean the request from \r and \n
    for (int i = 0; i < strlen(token); i++) {
        if (token[i] == '\r' || token[i] == '\n') {
            token[i] = '\0';
        }
    }
    req.args = calloc(strlen(token) + 1, sizeof(char));
    strncpy(req.args, token, strlen(token));
    return req;
}

void HandleRequest(int socket, OpenedSocket *data_socket, char *request) {
    bool command_found = false;
    // Parse the request
    Request req = ParseRequest(request);
    // Search in cmds[] the command and execute it
    // We can't use sizeof because it returns the size of the pointer
    for (int i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        // printf("Command: %s - %s\n", cmds[i].command, req.command);
        if (strcmp(cmds[i].command, req.command) == 0) {
            // print the thread, the source port and the request
            printf("Thread %lu: %s\n", pthread_self() % 100, req.command);
            cmds[i].function(socket, data_socket, req.args);
            command_found = true;
            break;
        }
    }
    if (!command_found) {
        SendOneLineCommand(socket, 502);
    }
    free(req.command);
    free(req.args);
}