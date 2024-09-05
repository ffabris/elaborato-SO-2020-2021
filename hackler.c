/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "defines.h"

int main(int argc, char * argv[]) {
    // Acquisizione e blocco dei segnali
    sigset_t mySet;
    sigfillset(&mySet);
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    // Variabili per la lettura di un file
    char line[B_MAX_CHAR];
    const char delim[] = ";";

    // Variabili per la gestione delle azioni
    unsigned int del;
    char target[3];
    char action[14];

    // Attesa della creazione di F8.csv e F9.csv
    while (access(F8, F_OK) == -1 || access(F9, F_OK) == -1);

    // Salvataggio PID senders da F8.csv
    pid_t senders[N_PROCS];
    const int f8_fd = open(F8, O_RDONLY);
    if (f8_fd == -1)
        ErrExit("Errore open(F8) in hackler.c");
    lseek(f8_fd, strlen(HEAD_F8), SEEK_SET);
    read_pids(f8_fd, senders);
    close(f8_fd);

    // Salvataggio PID receivers da F9.csv
    pid_t receivers[N_PROCS];
    const int f9_fd = open(F9, O_RDONLY);
    if (f9_fd == -1)
        ErrExit("Errore open(F9) in hackler.c");
    lseek(f9_fd, strlen(HEAD_F9), SEEK_SET);
    read_pids(f9_fd, receivers);
    close(f9_fd);

    // Apertura file F7.csv, ottenuto come argomento del programma
    const int f7_fd = open(argv[1], O_RDONLY);
    if (f7_fd == -1)
        ErrExit("Errore open(F7) in hackler.c");
    lseek(f7_fd, strlen(HEAD_F7), SEEK_SET);

    // Ciclo per la lettura e gestione delle azioni di disturbo
    do {
        // Lettura di una riga dal file
        if (read_line(f7_fd, line, sizeof line) <= 0)
            ErrExit("Errore read_line in hackler.c");

        // Memorizzazione della riga letta in act
        strtok(line, delim);
        del = atoi(strtok(NULL, delim));
        // NB: la riga contenente ShutDown ha tre campi invece di quattro
        char *token = strtok(NULL, delim);
        if (strcmp(token, "ShutDown") != 0) {
            strcpy(target, token);
            strcpy(action, strtok(NULL, delim));
        } else {
            target[0] = '\0';
            strcpy(action, token);
        }

        // Esecuzione della azione di disturbo (tramite un processo figlio)
        if (fork() != 0)
            continue;

        // Processo Figlio: gestione azione di disturbo

        // Attesa del processo
        sleep(del);

        // Memorizzo l'array su cui svolgere l'azione
        pid_t *arr_target;
        if (target[0] == 'S')
            arr_target = senders;
        else if (target[0] == 'R')
            arr_target = receivers;
        else
            arr_target = NULL;

        // Esecuzione azione richiesta
        if (arr_target && strcmp(action, "IncreaseDelay") == 0)
            kill(arr_target[target[1] - '1'], SIGUSR1);
        else if (arr_target && strcmp(action, "RemoveMSG") == 0)
            kill(arr_target[target[1] - '1'], SIGUSR2);
        else if (arr_target && strcmp(action, "SendMSG") == 0)
            kill(arr_target[target[1] - '1'], SIGALRM);
        else {
            // ShutDown: terminazione processi senders e receivers
            for (int i = 0; i < N_PROCS; ++i) {
                kill(senders[i], SIGTERM);
                kill(receivers[i], SIGTERM);
            }
        }

        // Terminazione processo
        _exit(EXIT_SUCCESS);
    } while (strcmp(action, "ShutDown") != 0);

    // Chiusura file F7.csv
    close(f7_fd);

    // Attesa della terminazione dei processi figli
    while (wait(NULL) != -1);

    // Terminazione hackler
    return 0;
}
