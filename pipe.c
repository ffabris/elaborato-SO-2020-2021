/// @file pipe.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle PIPE.

#include "err_exit.h"
#include "pipe.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int set_pipe(int pipe_fd[2]) {
    // Creazione della pipe
    if (pipe(pipe_fd) == -1)
        return -1;

    // Settaggio della flag O_NONBLOCK (e restituzione esito operazione)
    return fcntl(pipe_fd[0], F_SETFL, fcntl(pipe_fd[0], F_GETFL) | O_NONBLOCK);
}

void log_pipe(char *line, int pipe[2], char *name, char *src) {
    // Memorizzazione tempo di creazione
    char time_str[9];
    time_t rawtime;
    time(&rawtime);
    strftime(time_str, sizeof time_str, "%X", localtime(&rawtime));

    // Salvataggio informazioni della pipe nell'array
    sprintf(line, "%s;%d/%d;%s;%s", name, pipe[0], pipe[1], src, time_str);
}

void close_all_pipes(int arr_fd[][2], int arr_size) {
    for (int i = 0; i < arr_size; ++i)
        // Chiusura della i-esima pipe, sia in lettura che in scrittura
        if (close(arr_fd[i][0]) == -1 || close(arr_fd[i][1]) == -1)
            ErrExit("Errore close in close_all_pipes");
}
