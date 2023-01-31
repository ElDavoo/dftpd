//
// Created by Davide on 31/01/2023.
//

#ifndef DFTP_WORKERTHREAD_H
#define DFTP_WORKERTHREAD_H

/* La funzione che gestisce le connessioni */
void *ThreadMain(void *socket_desc);

/* Il numero massimo di caratteri in una richiesta */
#define MAX_SIZE 256

#endif //DFTP_WORKERTHREAD_H
