/* Guardia per evitare che il file venga incluso più volte */
#ifndef DFTP_FTPCOMMANDS_H
#define DFTP_FTPCOMMANDS_H

#include <pthread.h>
#include "SocketAperti.h"

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



void GestisciRichiesta(int socket, SocketAperto *data_socket, char *requ);

extern pthread_mutex_t file_to_rename_lock;
extern char *file_to_rename;
void MandaRisposta(int socket, int codiceRisposta);
long GetCurrentTime();

/* Definisce tutte le funzioni che gestiscono i comandi*/
void OnCwd(int socket, SocketAperto *data_socket, char *args);
void OnDele(int socket, SocketAperto *data_socket, char *nomeFile);
void OnFeat(int socket, SocketAperto *data_socket, char *args);
void OnMlsd(int socket, SocketAperto *data_socket, char *args);
void OnPasv(int socket, SocketAperto *data_socket, char *args);
void OnPwd(int socket, SocketAperto *data_socket, char *args);
void OnQuit(int socket, SocketAperto *data_socket, char *args);
void OnRetr(int socket, SocketAperto *data_socket, char *args);
void OnRnfr(int socket, SocketAperto *data_socket, char *args);
void OnRnto(int socket, SocketAperto *data_socket, char *args);
void OnStor(int socket, SocketAperto *data_socket, char *args);
void OnSyst(int socket, SocketAperto *data_socket, char *args);
void OnType(int socket, SocketAperto *data_socket, char *args);
void OnUser(int socket, SocketAperto *data_socket, char *args);

#endif //DFTP_FTPCOMMANDS_H
