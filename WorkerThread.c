//
// Created by Davide on 31/01/2023.
//

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <unistd.h>
#include "WorkerThread.h"
#include "OpenedSockets.h"
#include "FtpCommands.h"
#include "main.h"

void *ThreadMain(void *socket_desc) {
    int socket = *(int *) socket_desc;
    char client_request[BUFSIZ];

    // Stampa il mnessaggio di benvenuto alla connessione
    SendOneLineCommand(socket, 220);

    //init lock
    pthread_mutex_init(&lock, NULL);

    OpenedSocket data_socket;
    data_socket.socket = -1;
    data_socket.thread_id = -1;
    data_socket.open_port = 0;

    // Infinite loop to handle requests
    while (1) {
        ssize_t bytes_received = recv(socket, client_request, BUFSIZ - 1, 0);

        // Receive a message from client
        if (bytes_received < 0) {
            perror("recv failed");
            return NULL;
        } else if (bytes_received == 0) {
            printf("Client disconnected\n");
            // Cleanup: close all the sockets
            for (int i = 0; i < socketAperti->size; i++) {
                // Check if the socket has our thread id
                if (socketAperti->sockets[i].thread_id == pthread_self()) {
                    close(socketAperti->sockets[i].socket);
                    socketAperti->sockets[i].socket = -1;
                    socketAperti->sockets[i].thread_id = -1;
                }
            }

            break;
        }
        // Detect if the client has disconnected
        if (strlen(client_request) != 0) {



            // Handle the request
            HandleRequest(socket, &data_socket, client_request);
        }


    }

    free(socket_desc);
    //destroy lock
    pthread_mutex_destroy(&lock);
    return NULL;
}