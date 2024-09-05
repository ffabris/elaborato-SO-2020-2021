/// @file fifo.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione delle FIFO.

#pragma once

#include <fcntl.h>

// Crea il server FIFO, se aperto in scrittura (SM), e lo apre
int open_fifo(char *file_name, int flags);

// Registra l'apertura di una FIFO
void log_fifo(char *line, int fifo_fd, char *name, char *src);
