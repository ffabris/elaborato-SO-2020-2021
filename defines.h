/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include "err_exit.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Elenco file da scrivere
#define F1              "OutputFiles/F1.csv"
#define F2              "OutputFiles/F2.csv"
#define F3              "OutputFiles/F3.csv"
#define F4              "OutputFiles/F4.csv"
#define F5              "OutputFiles/F5.csv"
#define F6              "OutputFiles/F6.csv"
#define F8              "OutputFiles/F8.csv"
#define F9              "OutputFiles/F9.csv"
#define F10             "OutputFiles/F10.csv"

// Flags per i file da scrivere
#define F_FLGS          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

// Indirizzo file per il server fifo
#define FIFO_PATH       "OutputFiles/my_fifo.txt"

// Chiavi usati nel progetto
#define M_KEY           01101101
#define Q_KEY           01110001
#define S_KEY           01110011

// Numero massimo di caratteri salvabili in un array di messaggi
#define MSG_LEN         50

// Numero massimo di caratteri salvabili in un buffer
#define B_MAX_CHAR      50 + MSG_LEN

// Numero di processi figli presenti nel sender manager e nel receiver manager
#define N_PROCS         3

// Dimensione array di pipes e indice per sender manager e receiver manager
#define N_PIPES         2
#define PIPE1           0
#define PIPE2           1
#define PIPE3           1
#define PIPE4           0

// Tipi per l'invio / ricezione dei messaggi
#define TYPE_Q          1
#define TYPE_SH         2
#define TYPE_FIFO       3

// Numero massimo di strutture da allocare nella memoria condivisa
#define SH_MAX_MSGS     15

// Numero di semafori da gestire e indice dei semafori
#define N_SEMS          5
#define SEM_SH_AVL      0
#define SEM_MUT         1
#define SEM_F10         2
#define SEM_DEL         3
#define SEM_TERM        4

// Header dei file da leggere / scrivere
#define HEAD_F0     "ID;Message;IDSender;IDReceiver;DelS1;DelS2;DelS3;Type\n"
#define HEAD_F7     "ID;Delay;Target;Action\n"
#define HEAD_F8     "SenderID;PID\n"
#define HEAD_F9     "ReceiverID;PID\n"
#define HEAD_F10    "IPC;IDKey;Creator;CreationTime;DestructionTime\n"
#define HEAD_TR     "ID;Message;IDSender;IDReceiver;TimeArrival;TimeDeparture\n"

// Lettura di una porzione del file, delimitata da una andata a capo
ssize_t read_line(int fd, char string[], int str_len);

// Struttura usata per lavorare con i singoli messaggi
typedef struct message {
    long mtype;
    int id;
    int sender_id;
    int receiver_id;
    short type;
    unsigned int s_del[N_PROCS];
    char text[MSG_LEN + 1];
} message;

// Memorizza i valori ottenuti da una riga del file
ssize_t parse_message(int fd, message *msg);

// Struttura usata per gestire una lista di messaggi
typedef struct message_list {
    message msg;
    char log[B_MAX_CHAR];
    unsigned int del;
    struct message_list *prev;
    struct message_list *next;
} message_list;

// Aggiunge un messaggio in fondo alla lista
message_list *add_msg(message_list *head, message msg, unsigned int del);

// Rimuove un messaggio dalla lista
message_list *remove_msg(message_list *head, message_list *node);

// Svuota la lista dei messaggi
message_list *clear_msg_list(message_list *head);

// Prototipi per sender_manager.c e per receiver_manager.c

// Configura segnali usabili dal processo (SIGUSR1, SIGUSR2, SIGALRM, SIGTERM)
void set_sigs(sigset_t *set, __sighandler_t handler, short has_del);

// Scrive i pid dei processi figli in un file
void save_pid(char *file_name, pid_t arr_procs[], int n_proc, char label);

// Legge i pid dei processi figli da un file
void read_pids(int fd, pid_t arr[]);

// Crea il file per lo storico dei messaggi (e ne scrive l'intestazione)
int open_trinfo(char *file_name);

// Crea il file per lo storico delle ipc (e ne scrive l'intestazione)
void create_ipchist(char *file_name);

// Scrive una linea nello storico delle ipc
void write_ipchist(char *file_name, char *line);

// Registra la creazione di una struttura ipc
void log_ipc(char *line, int id, char *name, char *src);

// Registra la ricezione di un messaggio da parte di un processo
void log_arrival(char *line, message *msg);

// Registra il termine di una operazione
void log_end(char *line);
