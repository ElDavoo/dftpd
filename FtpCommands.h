/* Guardia per evitare che il file venga incluso più volte */
#ifndef DFTP_FTPCOMMANDS_H
#define DFTP_FTPCOMMANDS_H

#include "UserTable.h"
#include <pthread.h>
#include "OpenedSockets.h"

/* Una richiesta è costituita dal comando e dagli eventuali parametri */
typedef struct {
    char *comando;
    char *parametri;
} Richiesta;

/* Una risposta è costituita dallo stato e dal messaggio */
typedef struct {
    int stato;
    char *messaggio;
} Risposta;


Richiesta CreaRichiesta(char *request);



void GestisciRichiesta(int socket, OpenedSocket *data_socket, char *requ);

extern pthread_mutex_t lock;
void MandaRisposta(int socket, int codiceRisposta);

/* Definisce tutte le funzioni che gestiscono i comandi*/
void OnCwd(int socket, OpenedSocket *data_socket, char *args);
void OnDele(int socket, OpenedSocket *data_socket, char *args);
void OnFeat(int socket, OpenedSocket *data_socket, char *args);
void OnList(int socket, OpenedSocket *data_socket, char *args);
void OnPasv(int socket, OpenedSocket *data_socket, char *args);
void OnPwd(int socket, OpenedSocket *data_socket, char *args);
void OnQuit(int socket, OpenedSocket *data_socket, char *args);
void OnRetr(int socket, OpenedSocket *data_socket, char *args);
void OnRnfr(int socket, OpenedSocket *data_socket, char *args);
void OnRnto(int socket, OpenedSocket *data_socket, char *args);
void OnStor(int socket, OpenedSocket *data_socket, char *args);
void OnSyst(int socket, OpenedSocket *data_socket, char *args);
void OnType(int socket, OpenedSocket *data_socket, char *args);
void OnUser(int socket, OpenedSocket *data_socket, char *args);

#endif //DFTP_FTPCOMMANDS_H