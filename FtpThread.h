//
// Created by Davide on 31/01/2023.
//

#ifndef DFTP_FTPTHREAD_H
#define DFTP_FTPTHREAD_H

/* La funzione che gestisce le connessioni */
void *ThreadMain(void *socket_desc);

/* Il numero massimo di caratteri in una richiesta */
#define MAX_SIZE 256

/* Un comando è costituito dalla stringa che lo rappresenta e dalla funzione che lo gestisce */
typedef struct {
    char *comando;
    void (*function)(int socket, OpenedSocket *data_socket, char *args);
} Command;

#endif //DFTP_FTPTHREAD_H
