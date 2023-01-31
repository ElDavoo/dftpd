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

    if (CercaFile(tabellaFile, nomeFile) == -1) {

        MandaRisposta(socket, 550);
        return;

    }

    RimuoviFile(tabellaFile, nomeFile);
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

    /* Ottiene il file */
    FileVirtuale file = OttieniFile(tabellaFile, args);

    /* Annuncia l'inizio della trasmissione */
    MandaRisposta(socket, 150);

    /* Manda il file attraverso il socket dati */
    send(data_sk, file.contenuto, file.dimensione, 0);

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
    /* Controlla se esiste già un file con quel nome */
    if (CercaFile(tabellaFile, args) != -1) {
        free(file_to_rename);
        file_to_rename = NULL;
        pthread_mutex_unlock(&file_to_rename_lock);
        MandaRisposta(socket, 550);
        return;
    }

    /* Rinomina il file */
    RinominaFile(tabellaFile, file_to_rename, args);
    free(file_to_rename);
    file_to_rename = NULL;
    pthread_mutex_unlock(&file_to_rename_lock);

    MandaRisposta(socket, 250);
}

void OnSyst(int socket, SocketAperto *data_socket, char *args) {
    MandaRisposta(socket, 215);
}

void OnStor(int socket, SocketAperto *data_socket, char *args) {
    // Check if the data socket is open
    if (data_socket == NULL) {
        MandaRisposta(socket, 425);
        return;
    }
    // Check if the data socket is listening
    if (data_socket->socket == -1) {
        MandaRisposta(socket, 425);
        return;
    }
    // Accept the connection
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        MandaRisposta(socket, 425);
        return;
    }
    // Send the response
    MandaRisposta(socket, 150);
    // Receive the file here and save it into a string without calling functions
    char *file = malloc(1000000);
    // Fill with FF
    memset(file, 0xFF, 1000000);
    ssize_t file_size = 0;
    ssize_t bytes_read = 0;
    while ((bytes_read = recv(data_sk, file + file_size, 1000000 - file_size, 0)) > 0) {
        file_size += bytes_read;
    }
    file = realloc(file, file_size + 1);
    file[file_size] = '\0';
    printf("Thread %4lu:\tSTOR: Received %zd bytes\n", pthread_self() % 10000, file_size);
    // printf("Thread %d, STOR: FileVirtuale contenuto: %s\n", pthread_self() % 100, file);
    // Get the current time as YYYYMMDDHHMMSS.sss
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    strftime(s, sizeof(s), "%Y%m%d%H%M%S", tm);
    // as int
    long time = atol(s);

    RimuoviFile(tabellaFile, args);
    // Save the file into the file table
    FileVirtuale file_done = CreaFile(args, file_size, time, file);
    AggiungiFile(tabellaFile, file_done);

    // Close the data socket
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);
    shutdown(data_socket->socket, SHUT_RDWR);
    close(data_socket->socket);
    // Remove the data socket from the list
    RimuoviSocketAperto(socketAperti, data_socket->porta);
    data_socket->socket = -1;
    data_socket->porta = 0;
    // Send the response
    MandaRisposta(socket, 226);
    free(file);
}

void OnType(int socket, SocketAperto *data_socket, char *args) {
    MandaRisposta(socket, 200);
}

void OnUser(int socket, SocketAperto *data_socket, char *username) {
    MandaRisposta(socket, 230);
}


