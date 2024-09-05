/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "err_exit.h"
#include "fifo.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int open_fifo(char *file_name, int flags) {
    // Creazione file per la coda FIFO, se non esiste
    if (access(file_name, F_OK) == -1)
        mkfifo(file_name, S_IRUSR | S_IWUSR);

    // Aggiunta della flag O_NONBLOCK se richiesta una apertura in lettura
    if (flags == O_RDONLY)
        flags |= O_NONBLOCK;

    // Apertura file e restituzione del file descriptor
    return open(file_name, flags);
}

void log_fifo(char *line, int fifo_fd, char *name, char *src) {
    // Memorizzazione tempo di creazione
    char time_str[9];
    time_t rawtime;
    time(&rawtime);
    strftime(time_str, sizeof time_str, "%X", localtime(&rawtime));

    // Salvataggio informazioni della FIFO nell'array
    sprintf(line, "%s;%d;%s;%s", name, fifo_fd, src, time_str);
}
