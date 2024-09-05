/// @file shared_memory.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#pragma once

#include "defines.h"

#include <sys/shm.h>
#include <sys/stat.h>

// Crea la memoria condivisa da usare nel progetto
int set_shared_memory(key_t key, int n_elems);

// Invia il messaggio alla memoria condivisa (0: OK, -1: Errore)
int send_to_shared_memory(int shmid, message *msg, int index);

// Estrae il messaggio dalla memoria condivisa (0: OK, -1: Errore)
int read_shared_memory(int shmid, message *msg, int index);
