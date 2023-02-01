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
#include "SocketAperti.h"

#include "main.h"

/* Associa un comando con la funzione che lo gestisce in FtpCommands.c */
Command comandiGestiti[] = {
        {"CWD",  OnCwd},
        {"DELE", OnDele},
        {"FEAT", OnFeat},
        {"MLSD", OnMlsd},
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
void GestisciRichiesta(int socket, SocketAperto *data_socket, char *requ) {
    bool comandoTrovato = false;
    Richiesta richiesta = CreaRichiesta(requ);

    /* Ricerca nell'array dei comandi gestiti quello che è stato richiesto */
    for (int i = 0; i < sizeof(comandiGestiti) / sizeof(comandiGestiti[0]); i++) {

        if (strcmp(comandiGestiti[i].comando, richiesta.comando) == 0) {
            printf("Thread %4lu:\t%s\n", pthread_self() % 10000, richiesta.comando);
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

    /* Inizializza un socket dati aperto, locale per ogni thread */
    SocketAperto data_socket;
    data_socket.socket = -1;
    data_socket.idThread = pthread_self();
    data_socket.porta = 0;
    pthread_mutex_init(&data_socket.mutex, NULL);

    while (1) {

        /* Riceve al massimo MAX_SIZE caratteri dal client */
        ssize_t bytes_received = recv(socket, client_request, MAX_SIZE - 1, 0);

        if (bytes_received < 0) {

            perror("Thread: recv fallita");
            return NULL;

        } else if (bytes_received == 0) {

            printf("Thread %4lu:\tClient disconnesso, pulisco ed esco\n", pthread_self() % 10000);
            /* Controlla se c'era un file che stava venendo rinominato */
            if (file_to_rename != NULL) {
                free(file_to_rename);
                file_to_rename = NULL;
                pthread_mutex_unlock(&file_to_rename_lock);
            }

            /* Chiudo tutti i socket aperti dal thread */
            pthread_mutex_lock(&socketAperti->mutex);
            for (int i = 0; i < socketAperti->dimensione; i++) {

                if (socketAperti->sockets[i].idThread == pthread_self()) {

                    RimuoviChiudiSocketAperto(socketAperti, socketAperti->sockets[i].porta);

                }

            }
            pthread_mutex_unlock(&socketAperti->mutex);

            break;

        }

        if (strlen(client_request) != 0) {
            /* Gestisce la richiesta del client */
            GestisciRichiesta(socket, &data_socket, client_request);
        }

    }

    /* Distrugge il socket ed esce dal thread */
    free(socket_desc);
    pthread_mutex_destroy(&data_socket.mutex);
    return NULL;

}


