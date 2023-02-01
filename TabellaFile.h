/* Guardia per evitare che il file venga incluso più volte */
#ifndef DFTP_TABELLAFILE_H
#define DFTP_TABELLAFILE_H

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>

/* Definisce le variabili necessarie a risolvere il problema lettori-scrittory */
typedef struct {
    pthread_mutex_t mutex;
    sem_t s_lettori;
    sem_t s_scrittori;
    int lettori_attivi;
    bool scrittore_attivo;
    int lettori_bloccati;
    int scrittori_bloccati;
} LettoriScrittori;

/* Definisce un file, che è costituito da un nome, una dimensione, una data di modifica e il contenuto */
typedef struct {
    LettoriScrittori *sync;
    char *nome;
    unsigned long dimensione;
    long dataModifica;
    void *contenuto;
} FileVirtuale;

/* Definisce una tabella di file, quindi un array di FileVirtuali e la sua dimensione */
typedef struct {
    pthread_mutex_t mutex;
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

/* Ottiene la lista dei file in un formato adatto al comando MLSD */
char *ListaFileMlsd(TabellaFile *file_table);

/* Rimuove un file dalla tabella */
void RimuoviFile(TabellaFile *tf, char *name);

/* Rinomina un file */
void RinominaFile(TabellaFile *tf, char *nomeOriginale, char *nuovoNome);

/* Sovrascrive un file */
void SovrascriviFile(TabellaFile *tf, char *nomeFile, void *nuovoContenuto, size_t dimensioneNuovoContenuto);
#endif //DFTP_TABELLAFILE_H
