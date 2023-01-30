//
// Created by Davide on 30/01/2023.
//
#include <malloc.h>
#include "OpenedSockets.h"
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



OpenedSocket *CreateOpenedSocket(int socket) {
    OpenedSocket *os = malloc(sizeof(OpenedSocket));
    // Get the open port from the socket
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getsockname(socket, (struct sockaddr *) &addr, &addr_size);
    os->open_port = ntohs(addr.sin_port);
    os->socket = socket;
    // get the current thread id
    os->thread_id = (int) pthread_self();
    return os;
}

OpenedSockets *CreateOpenedSockets() {
    OpenedSockets *os = malloc(sizeof(OpenedSockets));
    os->size = 0;
    os->sockets = malloc(sizeof(OpenedSocket) * os->size);
    return os;
}

void AddOpenedSocket(OpenedSockets *os, OpenedSocket *socket) {
    os->size++;
    os->sockets = realloc(os->sockets, sizeof(OpenedSocket) * os->size);
    os->sockets[os->size - 1] = *socket;
}

int FindOpenedSocketByPort(OpenedSockets *os, int open_port) {
    for (int i = 0; i < os->size; i++) {
        if (os->sockets[i].open_port == open_port) {
            return i;
        }
    }
    return -1;
}

void RemoveOpenedSocket(OpenedSockets *os, int open_port) {
    int index = FindOpenedSocketByPort(os, open_port);
    if (index == -1) {
        return;
    }
    os->size--;
    os->sockets[index] = os->sockets[os->size];
    os->sockets = realloc(os->sockets, sizeof(OpenedSocket) * os->size);
}

void DestroyOpenedSockets(OpenedSockets *os) {
    free(os->sockets);
    free(os);
}
