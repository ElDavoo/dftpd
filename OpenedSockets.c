//
// Created by Davide on 30/01/2023.
//
#include "OpenedSockets.h"
#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>


unsigned short PortFromSocket(int socket) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getsockname(socket, (struct sockaddr *) &addr, &addr_size);
    unsigned short port = ntohs(addr.sin_port);
    return port;
}

OpenedSocket *CreateOpenedSocket(int socket) {
    OpenedSocket *os = malloc(sizeof(OpenedSocket));
    // Get the open port from the socket
    os->open_port = PortFromSocket(socket);
    os->socket = socket;
    // get the current thread id
    os->thread_id = (int) pthread_self();
    return os;
}


SocketAperti *CreateOpenedSockets() {
    SocketAperti *os = malloc(sizeof(SocketAperti));
    os->size = 0;
    os->sockets = NULL;
    return os;
}

void AddOpenedSocket(SocketAperti *os, OpenedSocket *socket) {
    os->size++;
    os->sockets = realloc(os->sockets, sizeof(OpenedSocket) * os->size);
    os->sockets[os->size - 1] = *socket;
}

int FindOpenedSocketByPort(SocketAperti *os, int open_port) {
    for (int i = 0; i < os->size; i++) {
        if (os->sockets[i].open_port == open_port) {
            return i;
        }
    }
    return -1;
}

void RemoveOpenedSocket(SocketAperti *os, int open_port) {
    int index = FindOpenedSocketByPort(os, open_port);
    if (index == -1) {
        return;
    }
    os->size--;
    os->sockets[index] = os->sockets[os->size];
    os->sockets = realloc(os->sockets, sizeof(OpenedSocket) * os->size);
}

void DestroyOpenedSockets(SocketAperti *os) {
    free(os->sockets);
    free(os);
}

void PrintOpenedSockets(SocketAperti *os) {
    printf("Opened sockets: %d\n", os->size);
    printf("thread_id\t\topen_port\tsocket\n");
    for (int i = 0; i < os->size; i++) {
        printf("%d\t\t%d\t\t%d\n", os->sockets[i].thread_id, ntohs(os->sockets[i].open_port), os->sockets[i].socket);
    }
}