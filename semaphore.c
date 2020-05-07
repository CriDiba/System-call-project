/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include <sys/sem.h>

#include "err_exit.h"
#include "semaphore.h"

void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {sem_num, sem_op, 0};

    if (semop(semid, &sop, 1) == -1)
        errExit("semop failed");
}
