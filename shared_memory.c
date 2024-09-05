/// @file shared_memory.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#include "err_exit.h"
#include "shared_memory.h"

int set_shared_memory(key_t key, int n_elems) {
    // Creazione memoria condivisa
    size_t size = n_elems * sizeof(message);
    int shmid = shmget(key, size, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid == -1)
        ErrExit("Errore shmget in set_shared_memory");

    // Restituzione id al chiamante
    return shmid;
}

int send_to_shared_memory(int shmid, message *msg, int index) {
    // Aggancio alla memoria condivisa
    message *ptr_shm = shmat(shmid, NULL, 0);
    if (ptr_shm == (void *) -1)
        return -1;

    // Inizializzazione della struttura nella memoria condivisa
    ptr_shm[index] = *msg;

    // Sgancio dalla memoria condivisa (e restituzione risultato)
    return shmdt(ptr_shm);
}

int read_shared_memory(int shmid, message *msg, int index) {
    // Aggancio alla memoria condivisa
    message *ptr_shm = shmat(shmid, NULL, SHM_RDONLY);
    if (ptr_shm == (void *) -1)
        return -1;

    // Copia dei valori ricavati dalla memoria condivisa
    *msg = ptr_shm[index];

    // Sgancio dalla memoria condivisa (e restituzione risultato)
    return shmdt(ptr_shm);
}
