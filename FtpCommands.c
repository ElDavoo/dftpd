#include "main.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <malloc.h>
#include "FtpCommands.h"
#include <stdlib.h>
#include <netinet/in.h>

#include "FileTable.h"

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
        {220, "david's ftp daemon v0.1!"}
};

/* Lista delle funzioni che verranno mandate col comando FEAT
 * Al momento non ci sono funzioni aggiuntive */
char *features[] = {};

char *file_to_rename = NULL;

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
    printf("Thread %lu\t\t: Errore: Risposta non trovata: %d\n", pthread_self(), codiceRisposta);
    MandaRisposta(socket, 502);
}

/* Implementazione dei comandi FTP */

void OnCwd(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the root directory is asked
    if (strcmp(args, "/") == 0) {
        MandaRisposta(socket, 250);
        return;
    }
    MandaRisposta(socket, 502);
}

void OnDele(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the file exists
    if (CercaFile(tabellaFile, args) == -1) {
        MandaRisposta(socket, 550);
        return;
    }
    // Remove the file from the file table
    RimuoviFile(tabellaFile, args);
    MandaRisposta(socket, 250);
}

void OnFeat(int socket, OpenedSocket *data_socket, char *args) {
    MandaRispostaIniziale(socket, 211, "Features:");
    for (int i = 0; i < sizeof(features) / sizeof(features[0]); i++) {
        // Prepend a space and add \r to the string
        char *feature = malloc(strlen(features[i]) + 2);
        strcpy(feature, " ");
        strcat(feature, features[i]);
        // add \r
        strcat(feature, "\r");
        MandaAlSocket(socket, feature);
        free(feature);
    }
    MandaRisposta(socket, 211);
}

void OnList(int socket, OpenedSocket *data_socket, char *args) {
    //printf("Before OnList\n");
    //PrintOpenedSockets(openedSockets);
    // Check if the data socket is open
    if (data_socket == NULL) {
        perror("data_socket == NULL");
        MandaRisposta(socket, 425);
        return;
    }
    // Check if the data socket is listening
    if (data_socket->socket == -1) {
        perror("data_socket->socket == -1");
        MandaRisposta(socket, 425);
        return;
    }
    // Accept the connection with timeout
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(data_socket->socket, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    // Send the response
    MandaRisposta(socket, 150);
    int data_sk = accept(data_socket->socket, (struct sockaddr *) &addr, &addr_len);
    if (data_sk == -1) {
        MandaRisposta(socket, 425);
        return;
    }

    // Send the list
    char *list = ListaFileMlsd(tabellaFile);
    send(data_sk, list, strlen(list), 0);
    free(list);
    // Close the data socket
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);
    shutdown(data_socket->socket, SHUT_RDWR);
    close(data_socket->socket);
    // Remove the data socket from the list
    RemoveOpenedSocket(socketAperti, data_socket->open_port);
    data_socket->socket = -1;
    data_socket->open_port = 0;
    // Send the response
    MandaRisposta(socket, 226);
    //printf("After OnList\n");
    //PrintOpenedSockets(openedSockets);
}

void OnPasv(int sk, OpenedSocket *data_socket, char *args) {
    //printf("Before OnPasv\n");
    //PrintOpenedSockets(openedSockets);
    // If the data socket is already in the list, remove it
    // If the data socket is already open, close it
    pthread_mutex_lock(&lock);
    if (data_socket->socket != -1 || data_socket->open_port != 0) {
        RemoveOpenedSocket(socketAperti, data_socket->open_port);
        close(data_socket->socket);
        //free(data_socket);
        //data_socket = NULL;
    }
    //data_socket = malloc(sizeof(OpenedSocket));
    data_socket->open_port = 0;
    data_socket->socket = -1;


    // Create the socket
    data_socket->socket = socket(AF_INET, SOCK_STREAM, 0);
    int optval = 1;
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(data_socket->socket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (data_socket->socket == -1) {
        MandaRisposta(sk, 500);
        perror("socket: ");
        shutdown(data_socket->socket, SHUT_RDWR);
        close(data_socket->socket);
        data_socket->socket = -1;
        data_socket->open_port = 0;
        pthread_mutex_unlock(&lock);
        return;
    }
    // Bind the socket
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    // Take a random port between 50000 and 60000
    int port = 50010 + rand() % 5;
    addr.sin_port = htons(port);
    // take the s_addr from sk
    struct sockaddr_in sk_addr;
    socklen_t sk_addr_len = sizeof(sk_addr);
    getsockname(sk, (struct sockaddr *) &sk_addr, &sk_addr_len);
    addr.sin_addr.s_addr = INADDR_ANY;
    while (bind(data_socket->socket, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind: ");
        // Take a random port between 50000 and 60000
        port = 50010 + rand() % 5;
        addr.sin_port = htons(port);
        data_socket->open_port = port;
    }
    // Listen
    if (listen(data_socket->socket, 1) == -1) {
        MandaRisposta(sk, 500);
        shutdown(data_socket->socket, SHUT_RDWR);
        close(data_socket->socket);
        data_socket->socket = -1;
        data_socket->open_port = 0;
        return;
    }
    printf("Thread %lu, PASV: Listening on port %d\n", pthread_self() % 100, port);

    // Send the response
    char *response = malloc(100);
    sprintf(response, "Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
            (int) (sk_addr.sin_addr.s_addr & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 8) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 16) & 0xFF),
            (int) ((sk_addr.sin_addr.s_addr >> 24) & 0xFF),
            (int) (port >> 8),
            (int) (port & 0xFF));
    MandaRispostaFinale(sk, 227, response);
    free(response);
    // Add the socket to the list of sockets
    AddOpenedSocket(socketAperti, data_socket);
    //printf("After OnPasv\n");
    //PrintOpenedSockets(openedSockets);
    pthread_mutex_unlock(&lock);

}

void OnPwd(int socket, OpenedSocket *data_socket, char *args) {
    MandaRisposta(socket, 257);
}

void OnQuit(int socket, OpenedSocket *data_socket, char *args) {
    MandaRisposta(socket, 221);
    // Disconnect the client
    close(socket);
}

void OnRetr(int socket, OpenedSocket *data_socket, char *args) {
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
    // Get the file from the file table
    FileVirtuale file = OttieniFile(tabellaFile, args);
    // Send the file
    send(data_sk, file.contenuto, file.dimensione, 0);
    // Close the data socket
    shutdown(data_sk, SHUT_RDWR);
    close(data_sk);
    shutdown(data_socket->socket, SHUT_RDWR);
    close(data_socket->socket);
    // Remove the data socket from the list
    RemoveOpenedSocket(socketAperti, data_socket->open_port);
    data_socket->socket = -1;
    data_socket->open_port = 0;
    // Send the response
    MandaRisposta(socket, 226);
}

void OnRnfr(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the file exists
    if (CercaFile(tabellaFile, args) == -1) {
        MandaRisposta(socket, 550);
        return;
    }
    // Save the file nome
    file_to_rename = calloc(strlen(args) + 1, sizeof(char));
    strncpy(file_to_rename, args, strlen(args));
    MandaRisposta(socket, 350);
}

void OnRnto(int socket, OpenedSocket *data_socket, char *args) {
    // Check if the file exists
    if (CercaFile(tabellaFile, file_to_rename) == -1) {
        MandaRisposta(socket, 550);
        return;
    }
    // Rename the file
    RinominaFile(tabellaFile, file_to_rename, args);
    free(file_to_rename);
    file_to_rename = NULL;
    MandaRisposta(socket, 250);
}

void OnSyst(int socket, OpenedSocket *data_socket, char *args) {
    MandaRisposta(socket, 215);
}

void OnStor(int socket, OpenedSocket *data_socket, char *args) {
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
    printf("Thread %lu, STOR: Received %zd bytes\n", pthread_self() % 100, file_size);
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
    RemoveOpenedSocket(socketAperti, data_socket->open_port);
    data_socket->socket = -1;
    data_socket->open_port = 0;
    // Send the response
    MandaRisposta(socket, 226);
    free(file);
}

void OnType(int socket, OpenedSocket *data_socket, char *args) {
    MandaRisposta(socket, 200);
}

void OnUser(int socket, OpenedSocket *data_socket, char *username) {
    MandaRisposta(socket, 230);
}


