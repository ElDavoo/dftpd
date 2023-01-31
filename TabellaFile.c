/* Implementa una semplice tabella di file
 * Questi file sono virtuali e risiedono solo nella memoria del server */
#include "TabellaFile.h"
#include <string.h>
#include <stdlib.h>

/* Crea una tabella di file vuota */
TabellaFile *CreaTabellaFile() {
    TabellaFile *tf = malloc(sizeof(TabellaFile));
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
    // Copia il nome in un nuovo puntatore e lo zero-termina
    file.nome = malloc(sizeof(char) * (strlen(nomeFile) + 1));
    strncpy(file.nome, nomeFile, strlen(nomeFile));
    file.nome[strlen(nomeFile)] = '\0';
    file.dimensione = dimensioneFile;
    file.dataModifica = dataModifica;
    // Copia il contenuto in un nuovo puntatore
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

/* Ottiene un FileVirtuale dalla tabella, se esiste. Altrimenti restituisce un FileVirtuale vuoto */
FileVirtuale OttieniFile(TabellaFile *tf, char *nomeFile) {
    int indice = CercaFile(tf, nomeFile);
    if (indice == -1) {
        FileVirtuale file;
        file.nome = NULL;
        file.dimensione = 0;
        return file;
    }
    return tf->files[indice];
}

/* Ottiene la lista dei file in un formato adatto al comando MLSD */
char *ListaFileMlsd(TabellaFile *file_table) {
    char *files_list = malloc(1);
    files_list[0] = '\0';
    for (int i = 0; i < file_table->dimensione; i++) {
        char file_info[100];
        sprintf(file_info, "type=file;dimensione=%ld;modify=%ld;perms=awr; %s\r\n", file_table->files[i].dimensione,
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

