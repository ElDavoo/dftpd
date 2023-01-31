/*
 * Questo file contiene il codice del thread che gestisce le richieste dei client
 */
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/socket.h>
#include <stdbool.h>
#include "FtpCommands.h"
#include "FtpThread.h"
#include "OpenedSockets.h"

#include "main.h"

/* Associa un comando con la funzione che lo gestisce in FtpCommands.c */
Command comandiGestiti[] = {
        {"CWD",  OnCwd},
        {"DELE", OnDele},
        {"FEAT", OnFeat},
        {"LIST", OnList},
        {"PASV", OnPasv},
        {"PWD",  OnPwd},
        {"QUIT", OnQuit},
        {"RETR", OnRetr},
        {"RNFR", OnRnfr},
        {"RNTO", OnRnto},
        {"STOR", OnStor},
        {"SYST", OnSyst},
        {"TYPE", OnType},
        {"USER", OnUser},

};

/* Cancella i caratteri di fine riga da una stringa */
void CancellaCaratteriFineRiga(char *token) {
    for (int i = 0; i < strlen(token); i++) {
        if (token[i] == '\r' || token[i] == '\n') {
            token[i] = '\0';
        }
    }
}

/* Crea una struttura Richiesta a partire da una stringa */
Richiesta CreaRichiesta(char *request) {

    /* Creiamo la struttura */
    Richiesta richiesta = {NULL, NULL};

    /* Estraiamo il comando */
    char *token = strtok(request, " ");

    /* Togliamo i caratteri di fine riga */
    CancellaCaratteriFineRiga(token);

    /* Copiamo il comando nella struttura */
    richiesta.comando = calloc(strlen(token) + 1, sizeof(char));
    strncpy(richiesta.comando, token, strlen(token));

    /* Ottieniamo il resto della stringa, che si suppone siano gli argomenti */
    token = request + strlen(token) + 1;
    CancellaCaratteriFineRiga(token);
    richiesta.parametri = calloc(strlen(token) + 1, sizeof(char));
    strncpy(richiesta.parametri, token, strlen(token));

    return richiesta;
}

/* Gestisce una singola richiesta */
void GestisciRichiesta(int socket, OpenedSocket *data_socket, char *requ) {
    bool comandoTrovato = false;
    Richiesta richiesta = CreaRichiesta(requ);

    /* Ricerca nell'array dei comandi gestiti quello che è stato richiesto */
    for (int i = 0; i < sizeof(comandiGestiti) / sizeof(comandiGestiti[0]); i++) {

        if (strcmp(comandiGestiti[i].comando, richiesta.comando) == 0) {
            printf("Thread %lu:\t\t%s\n", pthread_self() % 10000, richiesta.comando);
            /* Invoca la funzione che gestisce il comando */
            comandiGestiti[i].function(socket, data_socket, richiesta.parametri);
            comandoTrovato = true;
            break;
        }
    }
    if (!comandoTrovato) {
        MandaRisposta(socket, 502);
    }
    free(richiesta.comando);
    free(richiesta.parametri);
}

void *ThreadMain(void *socket_desc) {
    int socket = *(int *) socket_desc;
    char client_request[MAX_SIZE];

    // Stampa il messaggio di benvenuto alla connessione
    MandaRisposta(socket, 220);

    //init lock
    pthread_mutex_init(&lock, NULL);

    /* Inizializza un socket dati aperto, locale per ogni thread */
    OpenedSocket data_socket;
    data_socket.socket = -1;
    data_socket.thread_id = -1;
    data_socket.open_port = 0;

    while (1) {

        /* Riceve al massimo MAX_SIZE caratteri dal client */
        ssize_t bytes_received = recv(socket, client_request, MAX_SIZE - 1, 0);

        if (bytes_received < 0) {

            perror("Thread: recv fallita");
            return NULL;

        } else if (bytes_received == 0) {

            printf("Thread %lu: client disconnesso, pulisco ed esco\n", pthread_self() % 10000);

            /* Chiudo tutti i socket aperti dal thread */
            for (int i = 0; i < socketAperti->size; i++) {

                if (socketAperti->sockets[i].thread_id == pthread_self()) {

                    RemoveOpenedSocket(socketAperti, socketAperti->sockets[i].open_port);

                }

            }

            break;

        }

        if (strlen(client_request) != 0) {
            /* Gestisce la richiesta del client */
            GestisciRichiesta(socket, &data_socket, client_request);
        }


    }

    /* Distrugge il software ed esce dal thread */
    free(socket_desc);
    pthread_mutex_destroy(&lock);
    return NULL;

}


