/// @file receiver_manager.c
/// @brief Contiene l'implementazione del receiver_manager.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include "pipe.h"

// Variabili per la gestione IPC e log
int msg_queue, shmem, sem_set, server_fifo, fd_trinfo;

// Struttura per la gestione dei messaggi
message_list *msg_list = NULL;

// Gestione dei segnali ricevuti
void sigHandler(int sig) {
    // SIGTERM: setta SEM_TERM a 0, usata per continuare i cicli
    if (sig == SIGTERM && semctl(sem_set, SEM_TERM, GETVAL))
        set_sem(sem_set, SEM_TERM, 0);
}

// Funzione per la gestione della ricezione dei messaggi
void receive_msgs(int fd_rd, int *p_w, int receiver) {
    message temp;

    // Lettura dalla PIPE / FIFO passata con fd_rd
    if (read(fd_rd, &temp, sizeof temp) == sizeof temp)
        add_msg(msg_list, temp, 0);

    // Lettura dalla coda dei messaggi
    static ssize_t msg_size = sizeof(message) - sizeof(long);
    if (msgrcv(msg_queue, &temp, msg_size, receiver, IPC_NOWAIT) != -1)
        add_msg(msg_list, temp, 0);

    // Lettura dalla memoria condivisa
    semOp(sem_set, SEM_MUT, -1);
    int shm_i = SH_MAX_MSGS - semctl(sem_set, SEM_SH_AVL, GETVAL) - 1;
    if (shm_i >= 0 && read_shared_memory(shmem, &temp, shm_i) == 0) {
        // NB: memorizza il messaggio solo se è il destinatario
        if (temp.receiver_id == receiver) {
            msg_list = add_msg(msg_list, temp, 0);
            semOp(sem_set, SEM_SH_AVL, 1);
        }
    }
    semOp(sem_set, SEM_MUT, 1);

    // Gestioni messaggi raccolti dal receiver
    for (message_list *i = msg_list; i; i = i->next) {
        // Invio alla pipe se il processo non e' il destinatario
        if (i->msg.receiver_id != receiver) {
            if (p_w && write(p_w[1], &i->msg, sizeof i->msg) == sizeof i->msg)
                log_end(i->log);
        } else
            log_end(i->log);

        // Scrittura del messaggio nel file indicato da fd_trinfo
        strcat(i->log, "\n");
        write(fd_trinfo, i->log, strlen(i->log));
    }

    // Rimozione messaggi
    msg_list = clear_msg_list(msg_list);
}

int main(int argc, char * argv[]) {
    // Dichiarazione insieme dei segnali
    sigset_t mySet;

    // Array di stringhe per memorizzare la creazione delle IPC
    char ipc_str[N_PIPES][B_MAX_CHAR];
    char source[] = "RM";

    // Apertura FIFO in lettura
    server_fifo = open_fifo(FIFO_PATH, O_RDONLY);
    if (server_fifo == -1)
        ErrExit("Errore open_fifo in receiver_manager.c");

    // Acquisizione insieme dei semafori
    sem_set = semget(S_KEY, N_SEMS, S_IRUSR | S_IWUSR);
    if (sem_set == -1)
        ErrExit("Errore semget in receiver_manager.c");

    // Apertura coda di messaggi
    msg_queue = msgget(Q_KEY, S_IRUSR | S_IWUSR);
    if (msg_queue == -1)
        ErrExit("Errore msgget in receiver_manager.c");

    // Acquisizione memoria condivisa
    shmem = shmget(M_KEY, SH_MAX_MSGS * sizeof(message), S_IRUSR);
    if (shmem == -1)
        ErrExit("Errore shmget in receiver_manager.c");

    // File descriptors per le pipes
    int fd_p[N_PIPES][2];

    // Creazione PIPE3
    if (set_pipe(fd_p[PIPE3]) == -1)
        ErrExit("Errore set_pipe(PIPE3) in receiver_manager.c");
    log_pipe(ipc_str[0], fd_p[PIPE3], "PIPE3", source);

    // Creazione PIPE4
    if (set_pipe(fd_p[PIPE4]) == -1)
        ErrExit("Errore set_pipe(PIPE4) in receiver_manager.c");
    log_pipe(ipc_str[1], fd_p[PIPE4], "PIPE4", source);

    // Array dei processi receiver
    pid_t receivers[N_PROCS];

    // Processo R1
    receivers[0] = fork();
    if (receivers[0] == -1)
        ErrExit("Errore nella creazione di R1");
    else if (receivers[0] == 0) {
        // Configurazione segnali
        set_sigs(&mySet, sigHandler, 0);

        // Chiusura PIPE4 in scrittura e PIPE3 in lettura / scrittura
        if (close(fd_p[PIPE4][1]) == -1)
            ErrExit("Errore chiusura PIPE4 in scrittura");
        if (close(fd_p[PIPE3][0]) == -1 || close(fd_p[PIPE3][1]) == -1)
            ErrExit("Errore close PIPE3 in lettura / scrittura");

        // Scrittura (senza attesa) file F6.csv (solo intestazione)
        fd_trinfo = open_trinfo(F6);

        // Collegamento a PIPE4 e ricezione messaggi
        while (semctl(sem_set, SEM_TERM, GETVAL))
            receive_msgs(fd_p[PIPE4][0], NULL, 1);

        // Chiusura file indicato da fd_trinfo
        close(fd_trinfo);

        // Chiusura PIPE4 in lettura
        close(fd_p[PIPE4][0]);

        // Terminazione processo
        _exit(EXIT_SUCCESS);
    }

    // Processo R2
    receivers[1] = fork();
    if (receivers[1] == -1)
        ErrExit("Errore nella creazione di R2");
    else if (receivers[1] == 0) {
        // Configurazione segnali
        set_sigs(&mySet, sigHandler, 0);

        // Chiusura PIPE3 in scrittura e PIPE4 in lettura
        if (close(fd_p[PIPE3][1]) == -1)
            ErrExit("Errore chiusura PIPE3 in scrittura");
        if (close(fd_p[PIPE4][0]) == -1)
            ErrExit("Errore chiusura PIPE4 in lettura");

        // Scrittura (senza attesa) file F5.csv (solo intestazione)
        fd_trinfo = open_trinfo(F5);

        // Collegamento a PIPE3 e ricezione messaggi
        while (semctl(sem_set, SEM_TERM, GETVAL))
            receive_msgs(fd_p[PIPE3][0], fd_p[PIPE4], 2);

        // Chiusura file indicato da fd_trinfo
        close(fd_trinfo);

        // Chiusura PIPE3 in lettura e PIPE4 in scrittura
        close(fd_p[PIPE3][0]);
        close(fd_p[PIPE4][1]);

        // Terminazione processo
        _exit(EXIT_SUCCESS);
    }

    // Processo R3
    receivers[2] = fork();
    if (receivers[2] == -1)
        ErrExit("Errore nella creazione di R3");
    else if (receivers[2] == 0) {
        // Configurazione segnali
        set_sigs(&mySet, sigHandler, 0);

        // Chiusura PIPE3 in lettura e PIPE4 in lettura / scrittura
        if (close(fd_p[PIPE3][0]) == -1)
            ErrExit("Errore chiusura PIPE3 in lettura");
        if (close(fd_p[PIPE4][0]) == -1 || close(fd_p[PIPE4][1]) == -1)
            ErrExit("Errore close PIPE4 in lettura / scrittura");

        // Scrittura (senza attesa) file F4.csv (solo intestazione)
        fd_trinfo = open_trinfo(F4);

        // Collegamento a FIFO e ricezione messaggi
        while (semctl(sem_set, SEM_TERM, GETVAL))
            receive_msgs(server_fifo, fd_p[PIPE3], 3);

        // Chiusura file indicato da fd_trinfo
        close(fd_trinfo);

        // Chiusura PIPE3 in scrittura
        close(fd_p[PIPE3][1]);

        // Terminazione processo
        _exit(EXIT_SUCCESS);
    }

    // Processo genitore

    // Generazione file F9.csv
    save_pid(F9, receivers, N_PROCS, 'R');

    // Acquisizione e blocco dei segnali
    sigfillset(&mySet);
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    // Chiusura delle pipes
    close_all_pipes(fd_p, N_PIPES);

    // Attesa della terminazione dei figli
    pid_t child;
    while ((child = wait(NULL)) != -1) {
        if (child == receivers[1])
            log_end(ipc_str[1]);
        else if (child == receivers[2])
            log_end(ipc_str[0]);
    }

    // Chiusura server FIFO
    close(server_fifo);

    // Log delle pipes
    for (int i = 0; i < N_PIPES; ++i) {
        strcat(ipc_str[i], "\n");
        if (semOp(sem_set, SEM_F10, -1) == -1)
            ErrExit("Errore semOp(SEM_F10, -1) in receiver_manager.c");
        write_ipchist(F10, ipc_str[i]);
        if (semOp(sem_set, SEM_F10, 1) == -1)
            ErrExit("Errore semOp(SEM_F10, 1) in receiver_manager.c");
    }

    // Preparazione alla rimozione insieme dei semafori
    if (set_sem(sem_set, SEM_DEL, 0) == -1)
        ErrExit("Errore set_sem(SEM_DEL, -1) in receiver_manager.c");

    // Terminazione receiver manager
    return 0;
}
