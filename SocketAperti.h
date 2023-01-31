/* Guardia per evitare che il file venga incluso più volte */
#ifndef DFTP_SOCKETAPERTI_H
#define DFTP_SOCKETAPERTI_H

/* Un socket aperto è composto dall'id del thread che lo ha aperto, dalla porta e dal socket */
typedef struct {
    unsigned long idThread;
    unsigned short porta;
    int socket;
} SocketAperto;

typedef struct {
    int dimensione;
    SocketAperto *sockets;
} ListaSocketAperto;

/* Ottiene il numero di porta su cui sta ascoltando il socket */
unsigned short PortaDaSocket(int socket);

/* Crea una lista vuota di socket aperti */
ListaSocketAperto *CreaSocketAperti();

/* Crea un nuovo socket aperto a partire da un socket */
SocketAperto *CreaSocket(int socket);

/* Aggiunge un socket aperto alla lista */
void AddOpenedSocket(ListaSocketAperto *lista, SocketAperto *socket);

/* Restituisce il primo socket aperto con la porta specificata */
int CercaSocketConPorta(ListaSocketAperto *lista, int porta);

/* Rimuove un socket aperto dalla lista */
void RimuoviSocketAperto(ListaSocketAperto *lista, int porta);

#endif //DFTP_SOCKETAPERTI_H
