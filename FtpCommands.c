#include "main.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <malloc.h>
#include "FtpCommands.h"
#include <stdlib.h>
#include <netinet/in.h>

#include "TabellaFile.h"

/* Lista delle risposte che il server può inviare */
Risposta risposte[] = {
        {230, "User logged in, proceed"},
        {215, "UNIX Type: L8"},
        {211, "End"},
        {257, "\"/\" is current directory"},
        {200, "Command okay"},
        {350, "OK to rename file"},
        {226, "Closing data connection"},
        {150, "File stato okay; about to open data connection"},
        {550, "File unavailable"},
        {250, "Requested file action okay, completed"},
        {425, "Use PASV first."},
        {226, "Closing data connection"},
        {221, "Service closing control connection"},
        {500, "Syntax error, command unrecognized"},
        {502, "Command not implemented"},
        {503, "Wrong sequence of commands"},
        {220, "david's ftp daemon v0.1!"}
};

/* Lista delle funzioni che verranno mandate col comando FEAT */
char *features[] = { "MLST type*;size*;modify*;perm*;","MLSD", };

/* Il file che si sta per rinominare (nome specificato dal comando RNFR) */
char *file_to_rename = NULL;
pthread_mutex_t file_to_rename_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t lock;

/* Manda la stringa specificata al server. Aggiunge il Carriage Return \r */
void MandaAlSocket(int socket, char *comando) {
    /* Viene creata una stringa temporanea */
    size_t oldLunghezza = strlen(comando);
    char *comandoConCR = malloc(oldLunghezza + 2);
    strncpy(comandoConCR, comando, oldLunghezza);

    /* Viene aggiunto il carattere di fine riga e un null terminate */
    comandoConCR[oldLunghezza + 1] = '\0';
    comandoConCR[oldLunghezza] = '\r';

    /* Viene inviata la stringa e viene liberata la memoria */
    send(socket, comandoConCR, oldLunghezza + 1, 0);
    free(comandoConCR);
}

/* Manda la risposta specificata al socket specificato.
 * Nota che il client si aspetterà altre risposte. */
void MandaRispostaIniziale(int socket, int codiceRisposta, char *msg) {

    /* Creiamo una stringa temporanea per mettere il codice di stato e il messaggio */
    char *messaggio = malloc(strlen(msg) + 5);
    sprintf(messaggio, "%d-%s", codiceRisposta, msg);

    /* Inviamo la stringa e liberiamo la memoria */
    MandaAlSocket(socket, messaggio);
    free(messaggio);

}

/* Manda la risposta specificata al socket specificato.
 * Nota che il client non si aspetterà altre risposte. */
void MandaRispostaFinale(int socket, int codiceRisposta, char *msg) {

    /* Creiamo una stringa temporanea per mettere il codice di stato e il messaggio */
    char *messaggio = malloc(strlen(msg) + 5);
    sprintf(messaggio, "%d %s", codiceRisposta, msg);

    /* Inviamo la stringa e liberiamo la memoria */
    MandaAlSocket(socket, messaggio);
    free(messaggio);

}

/* Manda una risposta singola al socket specificato */
void MandaRisposta(int socket, int codiceRisposta) {
    /* Cerca la risposta nella lista delle risposte */
    for (int i = 0; i < sizeof(risposte) / sizeof(risposte[0]); i++) {
        if (risposte[i].stato == codiceRisposta) {
            MandaRispostaFinale(socket, codiceRisposta, risposte[i].messaggio);
            return;
        }
    }
    printf("Thread %4lu\t: Errore: Risposta non trovata: %d\n", pthread_self() % 10000, codiceRisposta);
    MandaRisposta(socket, 502);
}

/* Implementazione dei comandi FTP */

/* Comando CWD: Chiede cambio di directory */
void OnCwd(int socket, SocketAperto *data_socket, char *args) {

    // Se viene specificato il path /, allora va bene
    if (strcmp(args, "/") == 0) {

        MandaRisposta(socket, 250);
        return;

    }

    MandaRisposta(socket, 502);
}

/* Comando DELE: Chiede la cancellazione di un file */
void OnDele(int socket, SocketAperto *data_socket, char *nomeFile) {

    /* Blocca la tabella dei file */
    pthread_mutex_lock(&tabellaFile->mutex);

    int index = CercaFile(tabellaFile, nomeFile);
    if (index == -1) {
        pthread_mutex_unlock(&tabellaFile->mutex);
        MandaRisposta(socket, 550);
        return;

    }

    /* Cancelliamo il file solo se non è in uso da nessuno */
    pthread_mutex_lock(&tabellaFile->files[index].sync->mutex);
    if (tabellaFile->files[index].sync->scrittori_bloccati > 0 ||
        tabellaFile->files[index].sync->lettori_bloccati > 0 ||
        tabellaFile->files[index].sync->scrittore_attivo == true ||
        tabellaFile->files[index].sync->lettori_attivi > 0
        ) {
        pthread_mutex_unlock(&tabellaFile->files[index].sync->mutex);
        pthread_mutex_unlock(&tabellaFile->mutex);
        MandaRisposta(socket, 550);
        return;
    }

    /* Se non è in uso, allora lo cancelliamo */
    RimuoviFile(tabellaFile, nomeFile);

    pthread_mutex_unlock(&tabellaFile->mutex);

    MandaRisposta(socket, 250);

}

/* Comando FEAT: Chiede la lista delle funzioni supportate */
void OnFeat(int socket, SocketAperto *data_socket, char *args) {

    MandaRispostaIniziale(socket, 211, "Features:");

    /* Per ogni feature presente nell'array */
    for (int i = 0; i < sizeof(features) / sizeof(features[0]); i++) {

        /* Aggiunge uno spazio all'inizio e un carriage return, con una stringa temporanea */
        char *feature = malloc(strlen(features[i]) + 3);
        sprintf(feature, " %s\r", features[i]);
        feature[strlen(features[i]) + 2] = '\0';

        MandaAlSocket(socket, feature);

        free(feature);
    }

    /* Manda la risposta finale */
    MandaRisposta(socket, 211);

}

/* Comando MLSD: Chiede la lista dei file nella directory corrente */
void OnMlsd(int socket, SocketAperto *data_socket, char *args) {

    /* printf("Before OnList\n");
    StampaLista(openedSockets); */

    /* È necessario che ci sia una connessione dati aperta */
    if (data_socket == NULL) {
        printf("Thread %4lu:\tdata_socket == NULL", pthread_self() % 10000);
        MandaRisposta(socket, 425);
        return;
    }

    if (data_socket->socket == -1) {
        printf("Thread %4lu:\tdata_socket->socket == -1", pthread_self() % 10000);
        MandaRisposta(socket, 425);
        return;
    }

    /* Aggiunge un timeout di 10 secondi alla connessione dati */
    struct timeval tv;
    tv.tv_sec = 10;
    tv.tv_usec = 0;
    setsockopt(data_socket->socket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    /* Siamo pronti: mandiamo il segnale di inizio trasferimento dati */
    MandaRisposta(socket, 150);

    /* Accetta la connessione dati */
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        /* Connessione andata in timeout o altro genere di errore */
        printf("Thread %4lu:\tdata_sk == -1", pthread_self() % 10000);
        perror("\t\t");
        MandaRisposta(socket, 425);
        return;
    }

    /* Ottiene la lista dei file da mandare e la manda al server */
    char *list = ListaFileMlsd(tabellaFile);
    send(data_sk, list, strlen(list), 0);
    free(list);
    /* Chiude la singola connessione */
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);

    /* Smette di ascoltare sulla porta dati */
    RimuoviSocketAperto(socketAperti, data_socket->porta);
    data_socket->socket = -1;
    data_socket->porta = 0;

    /* Invia la risposta finale */
    MandaRisposta(socket, 226);
    //printf("After OnList\n");
    //StampaLista(openedSockets);

}

void OnPasv(int sk, SocketAperto *data_socket, char *args) {

    //printf("Before OnPasv\n");
    //StampaLista(openedSockets);

    pthread_mutex_lock(&lock);

    if (data_socket->porta != 0) {
        RimuoviSocketAperto(socketAperti, data_socket->porta);
        data_socket->porta = 0;
        data_socket->socket = -1;
    }

    /* Crea un nuovo socket */
    data_socket->socket = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    if (data_socket->socket == -1) {
        MandaRisposta(sk, 500);
        printf("Thread %4lu:\tdata_socket->socket == -1", pthread_self() % 10000);
        perror("\t\t");
        shutdown(data_socket->socket, SHUT_RDWR);
        close(data_socket->socket);
        data_socket->socket = -1;
        data_socket->porta = 0;
        pthread_mutex_unlock(&lock);
        return;
    }

    /* Inizializza i parametri del socket dati */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    /* Ottiene l'indirizzo IP del server dal socket di controllo */
    struct sockaddr_in sk_addr;
    socklen_t sk_addr_len = sizeof(sk_addr);
    getsockname(sk, (struct sockaddr *) &sk_addr, &sk_addr_len);

    while (1) {
        /* Genera un numero di porta casuale */
        data_socket->porta = 50010 + rand() % 5;
        /* Vede se c'è già un socket aperto con quella porta */
        int index = CercaSocketConPorta(socketAperti, data_socket->porta);
        if (index != -1) {
            printf("Thread %4lu:\tLa porta %d era già presa dal Thread %4lu\n", pthread_self() % 10000,
                   data_socket->porta, socketAperti->sockets[index].idThread % 10000);
            continue;
        }
        addr.sin_port = htons(data_socket->porta);
        int bindResult = bind(data_socket->socket, (struct sockaddr *) &addr, sizeof(addr));
        if (bindResult >= 0) {
            break;
        }
        perror("bind: ");
    }
    // Listen
    if (listen(data_socket->socket, 1) == -1) {
        MandaRisposta(sk, 500);
        shutdown(data_socket->socket, SHUT_RDWR);
        close(data_socket->socket);
        data_socket->socket = -1;
        data_socket->porta = 0;
        return;
    }

    /* Genera e manda la risposta al client */
    char *response = malloc(64);
    sprintf(response, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
            (int) (sk_addr.sin_addr.s_addr & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 8) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 16) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 24) & 0xFF),
            (int) (data_socket->porta >> 8),
            (int) (data_socket->porta & 0xFF));

    MandaRispostaFinale(sk, 227, response);
    free(response);

    /* Aggiunge il socket alla lista */
    AddOpenedSocket(socketAperti, data_socket);

    //printf("After OnPasv\n");
    //StampaLista(openedSockets);

    printf("Thread %4lu:\tPASV: Ascolto su porta %d\n", pthread_self() % 10000, data_socket->porta);

    pthread_mutex_unlock(&lock);

}

/* Comando PWD: restituisce il percorso corrente */
void OnPwd(int socket, SocketAperto *data_socket, char *args) {
    MandaRisposta(socket, 257);
}

/* Comando QUIT: chiude la connessione */
void OnQuit(int socket, SocketAperto *data_socket, char *args) {
    MandaRisposta(socket, 221);
    // Disconnect the client
    close(socket);
}

/* Comando RETR: invia un file al client */
void OnRetr(int socket, SocketAperto *data_socket, char *args) {
    if (data_socket == NULL) {
        MandaRisposta(socket, 425);
        return;
    }
    if (data_socket->socket == -1) {
        MandaRisposta(socket, 425);
        return;
    }

    /* Accetta la connessione */
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        MandaRisposta(socket, 425);
        return;
    }

    /* Blocchiamo la tabella dei file */
    pthread_mutex_lock(&tabellaFile->mutex);

    /* Cerchiamo il file */
    int index = CercaFile(tabellaFile, args);
    if (index == -1) {
        /* Il file non esiste */
        pthread_mutex_unlock(&tabellaFile->mutex);
        MandaRisposta(socket, 550);
        return;
    }

    /* Ottiene il file */
    FileVirtuale file = tabellaFile->files[index];

    LettoriScrittori *sync = file.sync;

    pthread_mutex_lock(&sync->mutex);
    if (!sync->scrittore_attivo && sync->scrittori_bloccati == 0 ){
        sync->lettori_attivi++;
        sem_post(&sync->s_lettori);
    } else {
        sync->lettori_bloccati++;
    }
    pthread_mutex_unlock(&sync->mutex);

    pthread_mutex_unlock(&tabellaFile->mutex);

    /* Aspetta che il file sia pronto */
    sem_wait(&sync->s_lettori);

    /* Annuncia l'inizio della trasmissione */
    MandaRisposta(socket, 150);

    /* Manda il file attraverso il socket dati */
    send(data_sk, file.contenuto, file.dimensione, 0);

    /* Blocchiamo la tabella dei file */
    pthread_mutex_lock(&tabellaFile->mutex);

    /* Cerchiamo il file */
    index = CercaFile(tabellaFile, args);
    if (index == -1) {
        /* Il file è stato cancellato */
        pthread_mutex_unlock(&tabellaFile->mutex);
        MandaRisposta(socket, 226);
        return;
    }

    sync = tabellaFile->files[index].sync;

    pthread_mutex_lock(&sync->mutex);
    sync->lettori_attivi--;
    if (sync->lettori_attivi == 0 && sync->scrittori_bloccati > 0) {
        sync->scrittore_attivo = true;
        sync ->scrittori_bloccati--;
        sem_post(&sync->s_scrittori);
    }
    pthread_mutex_unlock(&sync->mutex);

    pthread_mutex_unlock(&tabellaFile->mutex);


    /* Chiude il socket dati specifico */
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);

    /* Rimuove il socket dati dalla lista */
    RimuoviSocketAperto(socketAperti, data_socket->porta);
    data_socket->socket = -1;
    data_socket->porta = 0;

    /* Annuncia la fine della trasmissione */
    MandaRisposta(socket, 226);

}

/* Comando RNFR: Inizia a rinominare un file */
void OnRnfr(int socket, SocketAperto *data_socket, char *args) {
    /* Controlla se il file esiste */
    if (CercaFile(tabellaFile, args) == -1) {
        MandaRisposta(socket, 550);
        return;
    }
    /* Salva il nome del file nella variabile globale */
    pthread_mutex_lock(&file_to_rename_lock);
    file_to_rename = calloc(strlen(args) + 1, sizeof(char));
    strncpy(file_to_rename, args, strlen(args));
    MandaRisposta(socket, 350);
}

void OnRnto(int socket, SocketAperto *data_socket, char *args) {
    if (file_to_rename == NULL) {
        MandaRisposta(socket, 503);
        return;
    }
    pthread_mutex_lock(&tabellaFile->mutex);
    /* Controlla se esiste già un file con quel nome */
    if (CercaFile(tabellaFile, args) != -1) {
        pthread_mutex_unlock(&tabellaFile->mutex);
        free(file_to_rename);
        file_to_rename = NULL;
        pthread_mutex_unlock(&file_to_rename_lock);
        MandaRisposta(socket, 550);
        return;
    }

    pthread_mutex_lock(&tabellaFile->mutex);

    /* Rinomina il file */
    RinominaFile(tabellaFile, file_to_rename, args);

    pthread_mutex_unlock(&tabellaFile->mutex);

    free(file_to_rename);
    file_to_rename = NULL;
    pthread_mutex_unlock(&file_to_rename_lock);

    MandaRisposta(socket, 250);
}

/* Comando SYST: restituisce il sistema operativo */
void OnSyst(int socket, SocketAperto *data_socket, char *args) {
    MandaRisposta(socket, 215);
}

/* Ottiene la data corrente in formato YYYYMMDDHHMMSS */
long GetCurrentTime() {
    /* Il buffer in cui verrà memorizzata la data di modifica temporaneamente */
    char s[64];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(s, sizeof(s), "%Y%m%d%H%M%S", tm);
    long time = atol(s);
    return time;
}

/* Comando STOR: riceve un file dal client */
void OnStor(int socket, SocketAperto *data_socket, char *args) {

    /* Il buffer temporaneo in cui verrà salvato il file */
    char file[1000000];

    if (data_socket == NULL) {
        MandaRisposta(socket, 425);
        return;
    }

    if (data_socket->socket == -1) {
        MandaRisposta(socket, 425);
        return;
    }

    /* Accetta la connessione */
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        MandaRisposta(socket, 425);
        return;
    }

    ssize_t file_size = 0;
    ssize_t bytes_read = 0;

    /* Blocca la tabella dei file */
    pthread_mutex_lock(&tabellaFile->mutex);

    /* Cerca se esiste già un file con quel nome */
    int file_index = CercaFile(tabellaFile, args);
    /* Se non esiste, crea un file vuoto */
    if (file_index == -1) {
        FileVirtuale file_done = CreaFile(args, 0, 0, "");
        AggiungiFile(tabellaFile, file_done);
        file_index = CercaFile(tabellaFile, args);
    }

    LettoriScrittori *sync = tabellaFile->files[file_index].sync;

    /* Blocca il file */
    pthread_mutex_lock(&sync->mutex);
    if (sync->lettori_attivi == 0 && !sync->scrittore_attivo) {
        /* Signal al semaforo s_scrittori*/
        sem_post(&sync->s_scrittori);
        sync->scrittore_attivo = true;
    } else {
        sync->scrittori_bloccati++;
        /* In alternativa a far attendere il client, potrebbe
         * essere possibile rifiutare la richiesta
        pthread_mutex_unlock(&sync->mutex);
        pthread_mutex_unlock(&tabellaFile->mutex);
        MandaRisposta(socket, 550);
        return; */
    }
    /* Sblocca il file */
    pthread_mutex_unlock(&sync->mutex);


    /* Sblocca la tabella dei file */
    pthread_mutex_unlock(&tabellaFile->mutex);

    /* Aspetta il semaforo s_scrittori */
    sem_wait(&sync->s_scrittori);

    /* Segnala che siamo pronti a ricevere */
    MandaRisposta(socket, 150);

    /* Riceve e aggiorna il numero di byte ricevuti */
    while ((bytes_read = recv(data_sk, file + file_size, 1000000 - file_size, 0)) > 0) {
        file_size += bytes_read;
    }

    /* Mette un terminatore di stringa */
    file[file_size] = '\0';
    printf("Thread %4lu:\tSTOR: Received %zd bytes\n", pthread_self() % 10000, file_size);

    /* Ottiene la data di modifica */
    long time = GetCurrentTime();

    pthread_mutex_lock(&tabellaFile->mutex);

    sync = tabellaFile->files[file_index].sync;

    SovrascriviFile(tabellaFile, args, file, file_size);


    /* Possiamo sbloccare tutto */
    pthread_mutex_lock(&sync->mutex);
    sync->scrittore_attivo = false;
    if (sync->lettori_bloccati > 0) {
        do {
            sync->lettori_attivi++;
            sync->lettori_bloccati--;
            /* Signal al semaforo s_lettori */
            sem_post(&sync->s_lettori);
        } while (sync->lettori_bloccati != 0);

    } else if (sync->scrittori_bloccati > 0) {
        sync->scrittore_attivo = true;
        sync->scrittori_bloccati--;
        /* Signal al semaforo s_scrittori */
        sem_post(&sync->s_scrittori);
    }
    pthread_mutex_unlock(&sync->mutex);

    pthread_mutex_unlock(&tabellaFile->mutex);

    /* Chiude la connessione */
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);

    /* Rimuove il socket dalla lista */
    RimuoviSocketAperto(socketAperti, data_socket->porta);
    data_socket->socket = -1;
    data_socket->porta = 0;

    /* Manda la risposta finale */
    MandaRisposta(socket, 226);
}

/* Comando TYPE: imposta il tipo di trasferimento */
void OnType(int socket, SocketAperto *data_socket, char *args) {
    MandaRisposta(socket, 200);
}

/* Comando USER: imposta l'utente */
void OnUser(int socket, SocketAperto *data_socket, char *username) {
    MandaRisposta(socket, 230);
}


