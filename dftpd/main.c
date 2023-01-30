#include "main.h"
#include "FtpCommands.h"
#include "FileTable.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>

// Store the path to the users file without malloc
char users_file_path[1024];

void * HandleConnection(void *socket_desc);

void do_preauth_activities(int socket);

int SERVER_PORT = 2123;

OpenedSockets *openedSockets;

FileTable *file_table;

unsigned int mystate;

int main(int argc, char **argv) {
    mystate = time(NULL) ^ getpid() ^ pthread_self();
    srand(time(NULL));

    int socket_desc,
            socket_client,
            *new_sock,
            c = sizeof(struct sockaddr_in);
    openedSockets = CreateOpenedSockets();
    file_table = CreateFileTable();
    char cmd;
    struct sockaddr_in server,
            client;

    int addressToListen = INADDR_ANY;
    if (argc <= 1) {
        printf("Usage: %s [-p port]", argv[0]);
        return 1;
    }
    setbuf(stdout, NULL);
    // Print the pwd
    char cwd[1024] = {0};
    getcwd(cwd, sizeof(cwd));
    printf("Current working dir: %s", cwd);

    // Get options from command line with getopt
    while ((cmd = getopt(argc, argv, "hlp:u:")) != -1) {
        switch (cmd) {
            case 'h':
                printf("Usage: %s [-p port]", argv[0]);
                return 0;
            case 'l':
                addressToListen = INADDR_LOOPBACK;
                return 0;
            case 'p':
                SERVER_PORT = atoi(optarg);
                break;
            case 'u':
                strcat(users_file_path, cwd);
                strcat(users_file_path, optarg);
                break;
            case '?':
                if (optopt == 'p')
                    fprintf(stderr, "Option -%c requires an argument.", optopt);
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option `-%c'.", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.",
                            optopt);
                return 1;
            default:
                abort();
        }
    }


    // Create socket with reuseaddr and reuseport options
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (socket_desc == -1) {
        perror("Could not create socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = addressToListen;
    server.sin_port = htons(SERVER_PORT);

    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(socket_desc, 3);

    printf("Server started on port %d\n", SERVER_PORT);

    while ((socket_client = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c))) {
        if (socket_client < 0) {
            perror("Accept failed");
            return 1;
        }
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = socket_client;
        // printf("New connection from
        printf("New connection from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        pthread_create(&sniffer_thread, NULL, HandleConnection, (void *) new_sock);
        // instead of join, detach the thread
        pthread_detach(sniffer_thread);
    }
    return 0;
}

void * HandleConnection(void *socket_desc) {
    int socket = *(int *) socket_desc;
    char client_request[BUFSIZ];

    // setup socket in order to recv and send
    do_preauth_activities(socket);



    // Infinite loop to handle requests
    while (1) {
        ssize_t bytes_received = recv(socket, client_request, BUFSIZ, 0);

        // Receive a message from client
        if (bytes_received < 0) {
            perror("recv failed");
            return NULL;
        } else if (bytes_received == 0) {
            printf("Client disconnected\n");
            // Cleanup: close all the sockets
            for (int i = 0; i < openedSockets->size; i++) {
                // Check if the socket has our thread id
                if (openedSockets->sockets[i].thread_id == pthread_self()) {
                    close(openedSockets->sockets[i].socket);
                    openedSockets->sockets[i].socket = -1;
                    openedSockets->sockets[i].thread_id = -1;
                }
            }

            break;
        }
        // Detect if the client has disconnected
        if (strlen(client_request) != 0) {



            // Handle the request
            HandleRequest(socket, client_request);
        }




    }

    free(socket_desc);
    return NULL;
}

void do_preauth_activities(int socket) {
    char server_response[BUFSIZ];
    // Send 200-OK message to the client
    strcpy(server_response, "220-I delfanti\n");
    //write(socket, server_response, strlen(server_response));
    strcpy(server_response, "220 You are connected to the server\n");
    write(socket, server_response, strlen(server_response));
}
