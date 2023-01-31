/* Guardia per evitare che il file venga incluso più volte */
#ifndef DFTP_MAIN_H
#define DFTP_MAIN_H

#include "OpenedSockets.h"
#include "FileTable.h"

/* La lista dei socket aperti */
extern SocketAperti *socketAperti;

/* La tabella dei file */
extern TabellaFile *tabellaFile;

/* Lo stato del generatore di numeri casuali */
extern unsigned int mystate;



#endif //DFTP_MAIN_H
