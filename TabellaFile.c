/* Implementa una semplice tabella di file
 * Questi file sono virtuali e risiedono solo nella memoria del server */
#include "TabellaFile.h"
#include "FtpCommands.h"
#include <string.h>
#include <stdlib.h>

/* Crea una tabella di file vuota */
TabellaFile *CreaTabellaFile() {
    TabellaFile *tf = malloc(sizeof(TabellaFile));
    pthread_mutex_init(&tf->mutex, NULL);
    tf->dimensione = 0;
    tf->files = malloc(sizeof(FileVirtuale) * tf->dimensione);
    return tf;
}

/* Aggiunge un file alla tabella */
void AggiungiFile(TabellaFile *ft, FileVirtuale file) {
    ft->dimensione++;
    ft->files = realloc(ft->files, sizeof(FileVirtuale) * ft->dimensione);
    ft->files[ft->dimensione - 1] = file;
}

/* Crea un nuovo file */
FileVirtuale CreaFile(char *nomeFile, ssize_t dimensioneFile, long dataModifica, char *contenutoFile) {
    FileVirtuale file;
    /* Crea le strutture di sincronizzazzione */
    file.sync = malloc(sizeof(LettoriScrittori));
    pthread_mutex_init(&file.sync->mutex, NULL);
    /* Il valore iniziale di s_lettori e s_scrittori deve essere 0 */
    sem_init(&file.sync->s_lettori, 0, 0);
    sem_init(&file.sync->s_scrittori, 0, 0);
    file.sync->lettori_attivi = 0;
    file.sync->scrittore_attivo = false;
    file.sync->lettori_bloccati = 0;
    file.sync->scrittori_bloccati = 0;

    /* Copia il nome in un nuovo puntatore e lo zero-termina */
    file.nome = malloc(sizeof(char) * (strlen(nomeFile) + 1));
    strncpy(file.nome, nomeFile, strlen(nomeFile));
    file.nome[strlen(nomeFile)] = '\0';
    file.dimensione = dimensioneFile;
    file.dataModifica = dataModifica;
    /* Copia il contenuto in un nuovo puntatore */
    file.contenuto = calloc(strlen(contenutoFile) + 1, sizeof(char));
    strncpy(file.contenuto, contenutoFile, strlen(contenutoFile));
    return file;
}

/* Restituisce l'indice del file nella tabella, -1 se non esiste */
int CercaFile(TabellaFile *ft, char *nomeFile) {
    for (int i = 0; i < ft->dimensione; i++) {
        if (strcmp(ft->files[i].nome, nomeFile) == 0) {
            return i;
        }
    }
    return -1;
}

/* Ottiene la lista dei file in un formato adatto al comando MLSD */
char *ListaFileMlsd(TabellaFile *file_table) {
    char *files_list = malloc(1);
    files_list[0] = '\0';
    for (int i = 0; i < file_table->dimensione; i++) {
        char file_info[100];
        sprintf(file_info, "type=file;size=%ld;modify=%ld;perms=awr; %s\r\n", file_table->files[i].dimensione,
                file_table->files[i].dataModifica, file_table->files[i].nome);
        files_list = realloc(files_list, sizeof(char) * (strlen(files_list) + strlen(file_info) + 1));
        strncat(files_list, file_info, strlen(file_info));
    }
    return files_list;
}

/* Rimuove un file dalla tabella */
void RimuoviFile(TabellaFile *tf, char *name) {
    int indiceFile = CercaFile(tf, name);
    if (indiceFile == -1) {
        /* Non fare niente se il file non esiste */
        return;
    }
    tf->dimensione--;
    free(tf->files[indiceFile].nome);
    free(tf->files[indiceFile].contenuto);
    tf->files[indiceFile].nome = NULL;
    tf->files[indiceFile].contenuto = NULL;
    tf->files[indiceFile].dimensione = 0;
    tf->files[indiceFile].dataModifica = 0;

    /* Cancella le strutture di sincronizzazione */
    pthread_mutex_destroy(&tf->files[indiceFile].sync->mutex);
    sem_destroy(&tf->files[indiceFile].sync->s_lettori);
    sem_destroy(&tf->files[indiceFile].sync->s_scrittori);
    free(tf->files[indiceFile].sync);
    tf->files[indiceFile].sync = NULL;

    /* Sposta tutti i file successivi di una posizione indietro */
    for (int i = indiceFile; i < tf->dimensione; i++) {
        tf->files[i] = tf->files[i + 1];
    }
    tf->files = realloc(tf->files, sizeof(FileVirtuale) * tf->dimensione);
}

/* Rinomina un file */
void RinominaFile(TabellaFile *tf, char *nomeOriginale, char *nuovoNome) {
    int index = CercaFile(tf, nomeOriginale);
    if (index == -1) {
        /* Non fare niente se il file non esiste */
        return;
    }
    /* Butta via il vecchio nome e ne crea uno nuovo */
    free(tf->files[index].nome);
    tf->files[index].nome = calloc((strlen(nuovoNome) + 1), sizeof(char));
    strncpy(tf->files[index].nome, nuovoNome, strlen(nuovoNome));
    /* Si assicura che la stringa sia terminata */
    tf->files[index].nome[strlen(nuovoNome)] = '\0';
}

/* Sovrascrive un file con un nuovo contenuto */
void SovrascriviFile(TabellaFile *tf, char *nomeFile, void *nuovoContenuto, size_t dimensioneNuovoContenuto) {
    int index = CercaFile(tf, nomeFile);
    if (index == -1) {
        /* Non fare niente se il file non esiste */
        return;
    }
    /* Butta via il vecchio contenuto e ne crea uno nuovo */
    free(tf->files[index].contenuto);
    tf->files[index].contenuto = calloc(dimensioneNuovoContenuto + 1, sizeof(char));
    strncpy(tf->files[index].contenuto, nuovoContenuto, dimensioneNuovoContenuto);
    /* Si assicura che la stringa sia terminata */
    char *contenuto = tf->files[index].contenuto;
    contenuto[strlen(nuovoContenuto)] = '\0';
    tf->files[index].dimensione = dimensioneNuovoContenuto;
    tf->files[index].dataModifica = GetCurrentTime();

}

