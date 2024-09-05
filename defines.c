/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

ssize_t read_line(int fd, char string[], int str_len) {
    char letter;
    ssize_t bR, i;
    for (i = 0; i < str_len; ++i) {
        bR = read(fd, &letter, sizeof letter);

        // Controlla se la lettura è avvenuta correttamente e se si è letto \n
        if (bR == -1)
            return -1;
        else if (bR == 0 || letter == '\n' || letter == '\0') {
            string[i] = '\0';
            break;
        }

        // Memorizzazione del carattere nell'array
        string[i] = letter;
    }

    // Restituzione numero caratteri letti
    return i;
}

ssize_t parse_message(int fd, message *msg) {
    // Lettura di una riga del file
    char line[B_MAX_CHAR];
    ssize_t bR = read_line(fd, line, sizeof line);
    if (bR <= 0)
        return bR;

    // Copia della riga nel buffer locale, in vista della strtok
    char token[strlen(line)];
    const char delim[] = ";";
    strcpy(token, line);

    // Memorizzazione messaggio
    msg->id = atoi(strtok(token, delim));
    strncpy(msg->text, strtok(NULL, delim), MSG_LEN);
    msg->sender_id = strtok(NULL, delim)[1] - '0';
    msg->receiver_id = strtok(NULL, delim)[1] - '0';
    msg->mtype = msg->receiver_id;
    for (int i = 0; i < sizeof msg->s_del / sizeof(int); ++i)
        msg->s_del[i] = atoi(strtok(NULL, delim));

    // Memorizzazione tipo di gestione del messaggio
    char *line_type = strtok(NULL, delim);
    if (strcmp(line_type, "Q") == 0)
        msg->type = TYPE_Q;
    else if (strcmp(line_type, "SH") == 0)
        msg->type = TYPE_SH;
    else  // if (strcmp(line_type, "FIFO") == 0)
        msg->type = TYPE_FIFO;

    // Restituzione byte letti dalla read_line
    return bR;
}

message_list *add_msg(message_list *head, message msg, unsigned int del) {
    // Allocazione nuovo nodo nello heap
    message_list *node = malloc(sizeof(message_list));
    if (node == NULL)
        return head;

    // Inserimento del nodo nella lista
    if (head) {
        message_list *i;
        for (i = head; i->next; i = i->next);
        i->next = node;
        node->prev = i;
    } else {
        head = node;
        node->prev = NULL;
    }

    // Inizializzazione elementi del nodo
    node->msg = msg;
    log_arrival(node->log, &msg);
    node->del = del;
    node->next = NULL;

    // Restituzione testa della lista
    return head;
}

message_list *remove_msg(message_list *head, message_list *node) {
    // Se il nodo è la testa, faccio puntare head al successivo
    if (head == node)
        head = node->next;

    // Rimozione puntamento al nodo
    if (node->prev)
        node->prev->next = node->next;
    if (node->next)
        node->next->prev = node->prev;

    // Rimozione del nodo dallo heap
    free(node);

    // Restituzione testa della lista
    return head;
}

message_list *clear_msg_list(message_list *head) {
    message_list *temp;
    while (head) {
        temp = head;
        head = head->next;
        free(temp);
    }
    return NULL;
}

// Funzioni per sender_manager.c e per receiver_manager.c

void set_sigs(sigset_t *set, __sighandler_t handler, short has_del) {
    // Acquisizione dei segnali
    sigfillset(set);

    if (has_del) {
        // Configurazione SIGUSR1
        if (signal(SIGUSR1, handler) != SIG_ERR)
            sigdelset(set, SIGUSR1);
        else
            ErrExit("Errore signal(SIGUSR1) in set_handler");

        // Configurazione SIGUSR2
        if (signal(SIGUSR2, handler) != SIG_ERR)
            sigdelset(set, SIGUSR2);
        else
            ErrExit("Errore signal(SIGUSR2) in set_handler");

        // Configurazione SIGALRM
        if (signal(SIGALRM, handler) != SIG_ERR)
            sigdelset(set, SIGALRM);
        else
            ErrExit("Errore signal(SIGALRM) in set_handler");
    }

    // Configurazione SIGTERM
    if (signal(SIGTERM, handler) != SIG_ERR)
        sigdelset(set, SIGTERM);
    else
        ErrExit("Errore signal(SIGTERM) in set_handler");

    // Sblocco dei segnali configurati da set_sigs
    if (sigprocmask(SIG_SETMASK, set, NULL) == -1)
        ErrExit("Errore sigprocmask in set_handler");
}

void save_pid(char *file_name, pid_t arr_procs[], int n_proc, char label) {
    // Creazione file
    int fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, F_FLGS);
    if (fd == -1)
        ErrExit("Errore open in save_pid");

    // Scrittura intestazione su file
    char buffer[B_MAX_CHAR];
    if (label == 'S')
        sprintf(buffer, HEAD_F8);
    else if (label == 'R')
        sprintf(buffer, HEAD_F9);
    if (write(fd, buffer, strlen(buffer)) != strlen(buffer))
        ErrExit("Errore prima write in save_pid");

    // Scrittura su file
    for (int i = 0; i < n_proc; ++i) {
        sprintf(buffer, "%c%d;%d\n", label, i + 1, arr_procs[i]);
        if (write(fd, buffer, strlen(buffer)) != strlen(buffer))
            ErrExit("Errore seconda write in save_pid");
    }
    close(fd);
}

void read_pids(int fd, pid_t arr[]) {
    // Dichiarazione variabili
    char line[B_MAX_CHAR];
    int index;
    const char delim[] = ";";

    // Salvataggio pid nell'array arr
    while (read_line(fd, line, sizeof line) > 0) {
        index = strtok(line, delim)[1] - '0'- 1;
        arr[index] = atoi(strtok(NULL, delim));
    }
}

int open_trinfo(char *file_name) {
    // Creazione file
    int fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, F_FLGS);
    if (fd == -1)
        ErrExit("Errore open in open_trinfo");

    // Scrittura riga d'intestazione
    if (write(fd, HEAD_TR, strlen(HEAD_TR)) != strlen(HEAD_TR))
        ErrExit("Errore write in open_trinfo");

    // Restituzione file descriptor
    return fd;
}

void create_ipchist(char *file_name) {
    // Creazione e apertura file
    int fd = open(file_name, O_WRONLY | O_TRUNC | O_CREAT, F_FLGS);
    if (fd == -1)
        ErrExit("Errore open in open_ipchist");

    // Scrittura riga d'intestazione
    if (write(fd, HEAD_F10, strlen(HEAD_F10)) != strlen(HEAD_F10))
        ErrExit("Errore write in open_ipchist");

    // Chiusura file
    close(fd);
}

void write_ipchist(char *file_name, char *line) {
    // Apertura file
    int fd = open(file_name, O_WRONLY | O_APPEND);

    // Scrittura linea nel file
    write(fd, line, strlen(line));

    // Chiusura file
    close(fd);
}

void log_ipc(char *line, int id, char *name, char *src) {
    // Memorizzazione tempo di creazione
    char time_str[9];
    time_t rawtime;
    time(&rawtime);
    strftime(time_str, sizeof time_str, "%X", localtime(&rawtime));

    // Salvataggio informazioni della coda dei messaggi nell'array
    sprintf(line, "%s;%#X;%s;%s", name, id, src, time_str);

}

void log_arrival(char *line, message *msg) {
    // Memorizzazione tempo di arrivo
    char time_str[9];
    time_t rawtime;
    time(&rawtime);
    strftime(time_str, sizeof time_str, "%X", localtime(&rawtime));

    // Salvataggio informazioni messaggio nell'array
    sprintf(line, "%d;%s;%d;%d;%s", msg->id, msg->text,
            msg->sender_id, msg->receiver_id, time_str);

}

void log_end(char *line) {
    // Memorizzazione tempo di invio messaggio
    char time_str[10];
    time_t rawtime;
    time(&rawtime);
    strftime(time_str, sizeof time_str, ";%X", localtime(&rawtime));

    // Salvataggio tempo di invio nell'array
    strcat(line, time_str);
}
