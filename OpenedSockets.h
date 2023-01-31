//
// Created by Davide on 30/01/2023.
//

#ifndef DFTP_OPENEDSOCKETS_H
#define DFTP_OPENEDSOCKETS_H

// Implement a list of opened sockets
typedef struct {
    int thread_id;
    unsigned short open_port;
    int socket;
} OpenedSocket;

typedef struct {
    int size;
    OpenedSocket *sockets;
} SocketAperti;

unsigned short PortFromSocket(int socket);

SocketAperti *CreateOpenedSockets();

OpenedSocket *CreateOpenedSocket(int socket);

void AddOpenedSocket(SocketAperti *os, OpenedSocket *socket);

int FindOpenedSocketByPort(SocketAperti *os, int open_port);

void RemoveOpenedSocket(SocketAperti *os, int open_port);

void PrintOpenedSockets(SocketAperti *os);

#endif //DFTP_OPENEDSOCKETS_H
