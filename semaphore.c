/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"

int create_sems(key_t key, int n_sems, unsigned short values[]) {
    // Creazione insieme dei semafori
    int semid = semget(key, n_sems, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (semid == -1)
        ErrExit("Errore semget in create_sems");

    // Inizializzazione insieme
    union semun arg;
    arg.array = values;
    if (semctl(semid, 0, SETALL, arg) == -1)
        ErrExit("Errore semctl in create_sems");

    // Restituzione id dell'insieme
    return semid;
}

int set_sem(int semid, unsigned short sem_num, int sem_val) {
    // Inizializzazione insieme
    union semun arg;
    arg.val = sem_val;

    // Esecuzione operazione sul semaforo
    return semctl(semid, sem_num, SETVAL, arg);
}

int semOp(int semid, unsigned short sem_num, short sem_op) {
    // Struttura da passare alla semop
    struct sembuf sop = {
        .sem_num = sem_num,
        .sem_op = sem_op,
        .sem_flg = 0
    };

    // Esecuzione operazione sul semaforo
    return semop(semid, &sop, 1);
}
