//
// Created by Davide on 30/01/2023.
//

#ifndef DFTP_OPENEDSOCKETS_H
#define DFTP_OPENEDSOCKETS_H

// Implement a list of opened sockets
typedef struct {
    int thread_id;
    int open_port;
    int socket;
} OpenedSocket;

typedef struct {
    int size;
    OpenedSocket *sockets;
} OpenedSockets;

OpenedSockets *CreateOpenedSockets();
OpenedSocket *CreateOpenedSocket(int socket);
void AddOpenedSocket(OpenedSockets *os, OpenedSocket *socket);
int FindOpenedSocketByPort(OpenedSockets *os, int open_port);
void RemoveOpenedSocket(OpenedSockets *os, int open_port);
#endif //DFTP_OPENEDSOCKETS_H
