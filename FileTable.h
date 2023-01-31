/* Guardia per evitare che il file venga incluso più volte */
#ifndef DFTP_FILETABLE_H
#define DFTP_FILETABLE_H

#include <stdio.h>

/* Definisce un file, che è costituito da un nome, una dimensione, una data di modifica e il contenuto */
typedef struct {
    char *nome;
    ssize_t dimensione;
    long dataModifica;
    void *contenuto;
} FileVirtuale;

/* Definisce una tabella di file, quindi un array di FileVirtuali e la sua dimensione */
typedef struct {
    int dimensione;
    FileVirtuale *files;
} TabellaFile;

/* Crea una tabella di file vuota */
TabellaFile *CreaTabellaFile();

/* Aggiunge un file alla tabella */
void AggiungiFile(TabellaFile *ft, FileVirtuale file);

/* Restituisce l'indice del file nella tabella, -1 se non esiste */
FileVirtuale CreaFile(char *nomeFile, ssize_t dimensioneFile, long dataModifica, char *contenutoFile);

/* Crea un nuovo file */
int CercaFile(TabellaFile *ft, char *nomeFile);

/* Ottiene un FileVirtuale dalla tabella, se esiste. Altrimenti restituisce un FileVirtuale vuoto */
FileVirtuale OttieniFile(TabellaFile *tf, char *nomeFile);

/* Ottiene la lista dei file in un formato adatto al comando MLSD */
char *ListaFileMlsd(TabellaFile *file_table);

/* Rimuove un file dalla tabella */
void RimuoviFile(TabellaFile *tf, char *name);

/* Rinomina un file */
void RinominaFile(TabellaFile *tf, char *nomeOriginale, char *nuovoNome);

#endif //DFTP_FILETABLE_H
