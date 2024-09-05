/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

#include <sys/sem.h>
#include <sys/stat.h>

#ifndef _SEMUN_H
#define _SEMUN_H
// Struttura usata per le operazioni con i semafori
union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};
#endif

// Crea l'insieme dei semafori e lo inizializza
int create_sems(key_t key, int n_sems, unsigned short values[]);

// Inizializza il valore di un semaforo
int set_sem(int semid, unsigned short sem_num, int sem_val);

// Modifica il valore di un semaforo
int semOp(int semid, unsigned short sem_num, short sem_op);
