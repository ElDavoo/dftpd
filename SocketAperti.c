/* Definisce una lista di socket aperti e delle funzioni per gestirla */
#include "SocketAperti.h"
#include <stdio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

/* Ottiene il numero di porta su cui sta ascoltando il socket */
unsigned short PortaDaSocket(int socket) {
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    getsockname(socket, (struct sockaddr *) &addr, &addr_size);
    unsigned short port = ntohs(addr.sin_port);
    return port;
}

/* Crea una lista vuota di socket aperti */
ListaSocketAperto *CreaSocketAperti() {
    ListaSocketAperto *lista = malloc(sizeof(ListaSocketAperto));
    lista->dimensione = 0;
    lista->sockets = NULL;
    return lista;
}

/* Crea un nuovo socket aperto a partire da un socket */
SocketAperto *CreaSocket(int socket) {
    SocketAperto *os = malloc(sizeof(SocketAperto));
    // Get the open port from the socket
    os->porta = PortaDaSocket(socket);
    os->socket = socket;
    // get the current thread id
    os->idThread = pthread_self();
    return os;
}

/* Aggiunge un socket aperto alla lista */
void AddOpenedSocket(ListaSocketAperto *lista, SocketAperto *socket) {
    lista->dimensione++;
    lista->sockets = realloc(lista->sockets, sizeof(SocketAperto) * lista->dimensione);
    lista->sockets[lista->dimensione - 1] = *socket;
}

/* Restituisce il primo socket aperto con la porta specificata */
int CercaSocketConPorta(ListaSocketAperto *lista, int porta) {
    for (int i = 0; i < lista->dimensione; i++) {
        if (lista->sockets[i].porta == porta) {
            return i;
        }
    }
    return -1;
}

/* Rimuove un socket aperto dalla lista */
void RimuoviSocketAperto(ListaSocketAperto *lista, int porta) {
    int index = CercaSocketConPorta(lista, porta);
    if (index == -1) {
        return;
    }
    lista->dimensione--;
    /* Shutdown e spenge il socket */
    shutdown(lista->sockets[index].socket, SHUT_RDWR);
    close(lista->sockets[index].socket);
    /* Invalida i dati per sicurezza */
    lista->sockets[index].idThread = -1;
    lista->sockets[index].porta = -1;
    lista->sockets[index].socket = -1;

    /* Sposta tutti gli elementi successivi di una posizione indietro */
    for (int i = index; i < lista->dimensione; i++) {
        lista->sockets[i] = lista->sockets[i + 1];
    }
    lista->sockets = realloc(lista->sockets, sizeof(SocketAperto) * lista->dimensione);
}

/* Stampa una lista di socket aperti in quel momento */
void StampaLista(ListaSocketAperto *os) {
    printf("Socket aperti: %d\n", os->dimensione);
    printf("idThread\t\tport\tsocket\n");
    for (int i = 0; i < os->dimensione; i++) {
        printf("%lu\t%d\t\t%d\n", os->sockets[i].idThread, ntohs(os->sockets[i].porta), os->sockets[i].socket);
    }
}