/// @file pipe.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione delle PIPE.

#pragma once

// Crea una pipe e ne rende la lettura non bloccante
int set_pipe(int pipe_fd[2]);

// Registra la creazione di una pipe
void log_pipe(char *line, int pipe[2], char *name, char *src);

// Chiude le uscite in lettura / scrittura di tutte le pipes
void close_all_pipes(int arr_fd[][2], int arr_size);
