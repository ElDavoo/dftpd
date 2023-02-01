/* Guardia per evitare che il file venga incluso più volte */
#ifndef DFTP_MAIN_H
#define DFTP_MAIN_H

#include <pthread.h>
#include "SocketAperti.h"
#include "TabellaFile.h"

/* La lista dei socket aperti */
extern ListaSocketAperto *socketAperti;

/* La tabella dei file */
extern TabellaFile *tabellaFile;

#endif //DFTP_MAIN_H
