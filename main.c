/*
 * Questo file contiene il main del server
 */
#include "main.h"

#include "FtpThread.h"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

/* Puntatore globale alla lista dei socket dati aperti */
ListaSocketAperto *socketAperti;

/* Puntatore globale alla tabella dei file */
TabellaFile *tabellaFile;

int main(int argc, char **argv) {

    /* Disabilita il buffer del terminale, per evitare problemi di stampa */
    setbuf(stdout, NULL);
    printf("\n");

    int PORTA_SERVER = 2121;         /* La porta su cui il server ascolterà */

    srand(time(NULL));  /* Inizializzazione del generatore di numeri casuali */

    /* Variabili locali */
    int socket_client, socket_desc, *new_sock;
    struct sockaddr_in server, client = {0};
    int c = sizeof(struct sockaddr_in);

    /* Inizializzazione lista socket aperti e tabella file */
    socketAperti = CreaSocketAperti();
    tabellaFile = CreaTabellaFile();


    if (argc > 2) {
        printf("Utilizzo: %s porta\n", argv[0]);
        printf("Specificare solamente la porta.\n");
        printf("Se non specificata, il server ascolterà sulla porta 2121.\n");
        printf("Se si usa un numero di porta inferiore a 1024, è necessario essere root.\n");
        return 1;
    } else if (argc == 2) {
        PORTA_SERVER = atoi(argv[1]);
    }

    /* Crea un socket TCP IPv4, con le opzioni di riutilizzo indirizzo e porta */
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (socket_desc == -1) {
        perror("Main:\t\tCreazione socket fallita");
        return 1;
    }

    /* Impostazione parametri server e bind */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORTA_SERVER);

    if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Main:\t\tBind del server non riuscito");
        return 1;
    }

    if (listen(socket_desc, 100) < 0) { /* 100 è il numero massimo di connessioni in coda */
        perror("Main:\t\tListen fallito");
        return 1;
    }

    printf("Main:\t\tdftpd in ascolto sulla porta %d\n", PORTA_SERVER);

    while (true) {

        socket_client = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *) &c);
        if (socket_client < 0) {
            perror("Main:\t\tAccept fallito, uscita");
            return 1;
        }

        /* Dato che alla funzione thread è possibile passare solo un void*, è necessario allocare
         * un nuovo puntatore e passarlo. */
        new_sock = malloc(sizeof *new_sock);
        *new_sock = socket_client;

        /* Non ci è utile tenere riferimento al thread, quindi viene dichiarato qui */
        pthread_t sniffer_thread;
        printf("Main:\t\tNuova connessione da %s:%d - avvio un thread per gestirla\n",
               inet_ntoa(client.sin_addr),
               ntohs(client.sin_port));
        /* ThreadMain si trova in WorkerThread.c */
        pthread_create(&sniffer_thread, NULL, ThreadMain, new_sock);
        pthread_detach(sniffer_thread); /* Il thread termina automaticamente alla fine della funzione */

    }

}