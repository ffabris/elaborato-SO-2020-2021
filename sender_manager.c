/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"
#include "pipe.h"

// Orologio
int del_t = 0;

// Variabili per la gestione IPC e log
int msg_queue, shmem, sem_set, server_fifo, fd_trinfo;

// Struttura per la gestione dei messaggi
message_list *msg_list = NULL;

// Gestione dei segnali ricevuti
void sigHandler(int sig) {
    if (sig == SIGUSR1) {
        // SIGUSR1: incrementa di 5 i valori in msg_del
        for (message_list *i = msg_list; i; i = i->next)
            i->del += 5;
    } else if (sig == SIGUSR2) {
        // SIGUSR2: rimuove i messaggi
        msg_list = clear_msg_list(msg_list);
    } else if (sig == SIGALRM) {
        // SIGALRM: setta i valori in msg_del a 0
        for (message_list *i = msg_list; i; i = i->next)
            i->del = 0;
    } else if (sig == SIGTERM && semctl(sem_set, SEM_TERM, GETVAL)) {
        // SIGTERM: setta SEM_TERM a 0, usata per continuare i cicli
        set_sem(sem_set, SEM_TERM, 0);
    }
}

void send_msg(int *p_w, message_list *msg_i, int sender) {
    // Memorizzazione messaggio in una variabile
    message msg = msg_i->msg;

    // Invio alla pipe se il processo sender non e' il mittente
    if (msg_i->msg.sender_id != sender) {
        if (p_w && write(p_w[1], &msg, sizeof msg) == sizeof msg)
            log_end(msg_i->log);
    }
    // Altrimenti invio messaggio alla struttura appropriata
    else if (msg_i->msg.type == TYPE_Q) {
        static ssize_t msg_size = sizeof msg - sizeof(long);
        if (msgsnd(msg_queue, &msg, msg_size, 0) == 0)
            log_end(msg_i->log);
    } else if (msg_i->msg.type == TYPE_SH) {
        semOp(sem_set, SEM_SH_AVL, -1);
        int shm_i = SH_MAX_MSGS - semctl(sem_set, SEM_SH_AVL, GETVAL) - 1;
        if (send_to_shared_memory(shmem, &msg, shm_i) == 0)
            log_end(msg_i->log);
    } else if (msg_i->msg.type == TYPE_FIFO && sender == 3) {
        if (write(server_fifo, &msg, sizeof msg) == sizeof msg)
            log_end(msg_i->log);
    }

    // Scrittura del messaggio in fd_trinfo e rimozione dalla lista
    strcat(msg_i->log, "\n");
    write(fd_trinfo, msg_i->log, strlen(msg_i->log));
    msg_list = remove_msg(msg_list, msg_i);
}

int main(int argc, char * argv[]) {
    // Dichiarazione insieme dei segnali
    sigset_t mySet;

    // Array di stringhe per memorizzare la creazione delle IPC
    // NB: 0 = semaphore; 1 = msg_queue; 2 = sh; 3 = fifo; 4, 5 = pipes
    char ipc_str[N_PIPES + 4][B_MAX_CHAR];
    char source[] = "SM";

    // Creazione e inizializzazione insieme dei semafori
    unsigned short sem_vals[N_SEMS] = {SH_MAX_MSGS, 1, 1, 1, 1};
    sem_set = create_sems(S_KEY, N_SEMS, sem_vals);
    log_ipc(ipc_str[0], S_KEY, "SM", source);

    // Creazione coda di messaggi
    msg_queue = msgget(Q_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (msg_queue == -1)
        ErrExit("Errore msgget in sender_manager.c");
    log_ipc(ipc_str[1], Q_KEY, "Q", source);

    // Creazione memoria condivisa
    shmem = set_shared_memory(M_KEY, SH_MAX_MSGS);
    log_ipc(ipc_str[2], M_KEY, "SH", source);

    // File descriptors per le pipes
    int fd_p[N_PIPES][2];

    // Creazione PIPE1
    if (set_pipe(fd_p[PIPE1]) == -1)
        ErrExit("Errore set_pipe(PIPE1) in sender_manager.c");
    log_pipe(ipc_str[4], fd_p[PIPE1], "PIPE1", source);

    // Creazione PIPE2
    if (set_pipe(fd_p[PIPE2]) == -1)
        ErrExit("Errore set_pipe(PIPE2) in sender_manager.c");
    log_pipe(ipc_str[5], fd_p[PIPE2], "PIPE2", source);

    // Creazione file F10.csv
    create_ipchist(F10);

    // Apertura FIFO in scrittura
    server_fifo = open_fifo(FIFO_PATH, O_WRONLY);
    if (server_fifo == -1)
        ErrExit("Errore open_fifo in sender_manager.c");
    log_fifo(ipc_str[3], server_fifo, "FIFO", source);

    // Array dei processi sender
    pid_t senders[N_PROCS];

    // Processo S1
    senders[0] = fork();
    if (senders[0] == -1)
        ErrExit("Errore nella creazione di S1");
    else if (senders[0] == 0) {
        // Configurazione segnali
        set_sigs(&mySet, sigHandler, 1);

        // Chiusura PIPE1 in lettura e PIPE2 in lettura / scrittura
        if (close(fd_p[PIPE1][0]) == -1)
            ErrExit("Errore close PIPE1 in lettura");
        if (close(fd_p[PIPE2][0]) == -1 || close(fd_p[PIPE2][1]) == -1)
            ErrExit("Errore close PIPE2 in lettura / scrittura");

        // Apertura file F0.csv, ottenuto come argomento del programma
        int f0_fd = open(argv[1], O_RDONLY);
        if (f0_fd == -1)
            ErrExit("Errore open in sender_manager.c");

        // Apertura file F1.csv
        fd_trinfo = open_trinfo(F1);

        // Lettura file F0.csv
        lseek(f0_fd, strlen(HEAD_F0), SEEK_SET);
        message temp;
        while (parse_message(f0_fd, &temp) > 0)
            msg_list = add_msg(msg_list, temp, temp.s_del[0] + del_t);

        // Chiusura file F0.csv
        close(f0_fd);

        // invio dei messaggi
        while (semctl(sem_set, SEM_TERM, GETVAL)) {
            // Invio del messaggio, se possibile
            for (message_list *i = msg_list; i; i = i->next)
                if (i->del <= del_t)
                    send_msg(fd_p[PIPE1], i, 1);

            // Attesa e incremento del_t
            if (sleep(1) == 0)
                ++del_t;
        }

        // Chiusura file indicato da fd_trinfo
        close(fd_trinfo);

        // Chiusura PIPE1 in scrittura
        close(fd_p[PIPE1][1]);

        // Terminazione processo
        _exit(EXIT_SUCCESS);
    }

    // Processo S2
    senders[1] = fork();
    if (senders[1] == -1)
        ErrExit("Errore nella creazione di S2");
    else if (senders[1] == 0) {
        // Configurazione segnali
        set_sigs(&mySet, sigHandler, 1);

        // Chiusura PIPE1 in scrittura e PIPE2 in lettura
        if (close(fd_p[PIPE1][1]) == -1)
            ErrExit("Errore close PIPE1 in scrittura");
        if (close(fd_p[PIPE2][0]) == -1)
            ErrExit("Errore close PIPE2 in lettura");

        // Lettura da PIPE1 e scrittura su PIPE2
        fd_trinfo = open_trinfo(F2);

        // Gestione dei messaggi
        message temp;
        while (semctl(sem_set, SEM_TERM, GETVAL)) {
            // Lettura da PIPE1
            ssize_t bR = read(fd_p[PIPE1][0], &temp, sizeof temp);
            if (bR == sizeof temp)
                msg_list = add_msg(msg_list, temp, temp.s_del[1] + del_t);

            // Invio del messaggio, se possibile
            for (message_list *i = msg_list; i; i = i->next)
                if (i->del <= del_t)
                    send_msg(fd_p[PIPE2], i, 2);

            // Attesa e incremento del_t
            if (sleep(1) == 0)
                ++del_t;
        }

        // Chiusura file indicato da fd_trinfo
        close(fd_trinfo);

        // Chiusura PIPE1 in lettura e PIPE2 in scrittura
        close(fd_p[PIPE1][0]);
        close(fd_p[PIPE2][1]);

        // Terminazione processo
        _exit(EXIT_SUCCESS);
    }

    // Processo S3
    senders[2] = fork();
    if (senders[2] == -1)
        ErrExit("Errore nella creazione di S3");
    else if (senders[2] == 0) {
        // Configurazione segnali
        set_sigs(&mySet, sigHandler, 1);

        // Chiusura PIPE2 in scrittura e PIPE1 in lettura / scrittura
        if (close(fd_p[PIPE2][1]) == -1)
            ErrExit("Errore close PIPE2 in scrittura");
        if (close(fd_p[PIPE1][0]) == -1 || close(fd_p[PIPE1][1]) == -1)
            ErrExit("Errore close PIPE1 in lettura / scrittura");

        // Lettura da PIPE2 e scrittura su FIFO
        fd_trinfo = open_trinfo(F3);

        // Gestione dei messaggi
        message temp;
        while (semctl(sem_set, SEM_TERM, GETVAL)) {
            // Lettura da PIPE2
            ssize_t bR = read(fd_p[PIPE2][0], &temp, sizeof temp);
            if (bR == sizeof temp)
                msg_list = add_msg(msg_list, temp, temp.s_del[2] + del_t);

            // Invio del messaggio, se possibile
            for (message_list *i = msg_list; i; i = i->next)
                if (i->del <= del_t)
                    send_msg(fd_p[PIPE3], i, 3);

            // Attesa e incremento del_t
            if (sleep(1) == 0)
                ++del_t;
        }

        // Chiusura file indicato da fd_trinfo
        close(fd_trinfo);

        // Chiusura PIPE2 in lettura
        close(fd_p[PIPE2][0]);

        // Terminazione processo
        _exit(EXIT_SUCCESS);
    }

    // Processo genitore

    // Generazione file F8.csv
    save_pid(F8, senders, N_PROCS, 'S');

    // Acquisizione e blocco dei segnali
    sigfillset(&mySet);
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    // Chiusura delle pipes
    close_all_pipes(fd_p, N_PIPES);

    // Attesa della terminazione dei figli
    pid_t child;
    while ((child = wait(NULL)) != -1) {
        if (child == senders[0])
            log_end(ipc_str[4]);
        else if (child == senders[1])
            log_end(ipc_str[5]);
    }

    // Rimozione coda dei messaggi
    if (msgctl(msg_queue, IPC_RMID, NULL) == 0)
        log_end(ipc_str[1]);

    // Rimozione memoria condivisa
    if (shmctl(shmem, IPC_RMID, NULL) == 0)
        log_end(ipc_str[2]);

    // Chiusura server FIFO
    close(server_fifo);
    unlink(FIFO_PATH);
    log_end(ipc_str[3]);

    // Scrittura di ipc_str nel file F10
    const int n_str = sizeof ipc_str / sizeof(char[B_MAX_CHAR]);
    for (int i = 1; i < n_str; ++i) {
        strcat(ipc_str[i], "\n");
        if (semOp(sem_set, SEM_F10, -1) == -1)
            ErrExit("Errore semOp(SEM_F10, -1) in sender_manager.c");
        write_ipchist(F10, ipc_str[i]);
        if (semOp(sem_set, SEM_F10, 1) == -1)
            ErrExit("Errore semOp(SEM_F10, 1) in sender_manager.c");
    }

    // Attesa e rimozione insieme dei semafori
    if (semOp(sem_set, SEM_DEL, 0) == -1)
        ErrExit("Errore semOp(SEM_DEL, 0) in sender_manager.c");
    if (semctl(sem_set, 0, IPC_RMID, 0) == -1)
        ErrExit("Errore semctl(IPC_RMID) in sender_manager.c");
    log_end(ipc_str[0]);
    strcat(ipc_str[0], "\n");
    write_ipchist(F10, ipc_str[0]);

    // Terminazione sender manager
    return 0;
}
