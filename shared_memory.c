/// @file shared_memory.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione della MEMORIA CONDIVISA.

#include "err_exit.h"
#include "shared_memory.h"

#include <sys/shm.h>
#include <sys/stat.h>


 int alloc_shared_memory(key_t shmKey, size_t size) {
    // get, or create, a shared memory segment
    int shmid = shmget(shmKey, size, IPC_CREAT | S_IRUSR | S_IWUSR);
    if (shmid == -1)
        errExit("shmget failed");

    return shmid;
}

void *get_shared_memory(int shmid, int shmflg) {
    // attach the shared memory
    void *ptr_sh = shmat(shmid, NULL, shmflg);
    if (ptr_sh == (void *)-1)
        errExit("shmat failed");

    return ptr_sh;
}

void free_shared_memory(void *ptr_sh) {
    // detach the shared memory segments
    if (shmdt(ptr_sh) == -1)
        errExit("shmdt failed");
}

void remove_shared_memory(int shmid) {
    // delete the shared memory segment
    if (shmctl(shmid, IPC_RMID, NULL) == -1)
        errExit("shmctl failed");
}
