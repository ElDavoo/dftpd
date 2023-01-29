#include "main.h"
#include "FtpCommands.h"

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

// Store the path to the users file without malloc
char users_file_path[1024];

void *HandleConnection(void *socket_desc);

void do_preauth_activities(int socket);

int SERVER_PORT = 2123;

int main(int argc, char **argv) {
    int socket_desc,
            socket_client,
            *new_sock,
            c = sizeof(struct sockaddr_in);

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
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(SERVER_PORT);

    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(socket_desc, 3);

    while (socket_client = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c)) {
        if (socket_client < 0) {
            perror("Accept failed");
            return 1;
        }
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = socket_client;
        pthread_create(&sniffer_thread, NULL, HandleConnection, (void *) new_sock);
        pthread_join(sniffer_thread, NULL);
    }

    if (socket_client < 0) {
        perror("Accept failed");
        return 1;
    }

    return 0;
}

void *HandleConnection(void *socket_desc) {
    int socket = *(int *) socket_desc;
    char server_response[BUFSIZ],
            client_request[BUFSIZ],
            file_name[BUFSIZ];

    // setup socket in order to recv and send
    do_preauth_activities(socket);



    // Infinite loop to handle requests
    while (1) {
        // Receive a message from client
        if (recv(socket, client_request, BUFSIZ, 0) <= 0) {
            perror("recv failed");
            return 0;
        }
        // Detect if the client has disconnected
        if (strlen(client_request) == 0) {
            printf("Client disconnected");
            break;
        }

        printf("Client request: %s", client_request);
        // Handle the request
        HandleRequest(socket, client_request);


    }

    free(socket_desc);
    return 0;
}

void do_preauth_activities(int socket) {
    char server_response[BUFSIZ];
    // Send 200-OK message to the client
    strcpy(server_response, "220-I delfanti\n");
    //write(socket, server_response, strlen(server_response));
    strcpy(server_response, "220 You are connected to the server\n");
    write(socket, server_response, strlen(server_response));
}
