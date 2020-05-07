/// @file server.c
/// @brief Contiene l'implementazione del SERVER.
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"

void signTermHandler(int sig) {

    //Rimuovo IPC

    // terminate the server process
    exit(0);
}


int main(int argc, char * argv[]) {

    // check command line input arguments
    if (argc != 3) {
        printf("Usage: %s msg_queue_key file_posizioni\n", argv[0]);
        exit(1);
    }

    // read the message queue key defined by user
    int msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The message queue key must be greater than zero!\n");
        exit(1);
    }

    //Leggo il file file_posizioni
    int file_posizioni = open(argv[2], O_RDONLY);
    if (file_posizioni == -1) {
        printf("File %s does not exist\n", argv[2]);
        exit(1);
    }

    // set of signals (N.B. it is not initialized!)
    sigset_t mySet;
    // initialize mySet to contain all signals
    sigfillset(&mySet);
    // remove SIGTERM from mySet
    sigdelset(&mySet, SIGTERM);
    // blocking all signals but SIGTERM
    sigprocmask(SIG_SETMASK, &mySet, NULL);

    // set the function sigHandler as handler for the signal SIGTERM
    if(signal(SIGTERM, signTermHandler) == SIG_ERR)
        errExit("change signal handler failed");

    /*---------------------------------------------
        MEMORIA CONDIVISA
    -----------------------------------------------*/

    //GENERA MEMEORIA CONDIVISA Scacchiera
    int shmidBoard = alloc_shared_memory(IPC_PRIVATE, sizeof(struct Board));

    //Genera memoria CONDIVISA ACK
    int shmidAckList = alloc_shared_memory(IPC_PRIVATE, 100*sizeof(struct Acknowledgment));

    struct Acknowledgment *acknowled_list = (struct Acknowledgment*)get_shared_memory(shmidAckList, 0);

    /*---------------------------------------------
        SEMAFORI
    -----------------------------------------------*/

    // Genera semafori per movimento
    // Create a semaphore set with 5 semaphores
    int sem_idx_board = semget(IPC_PRIVATE, 5, S_IRUSR | S_IWUSR);
    if (sem_idx_board == -1)
        errExit("semget failed");

    // Initialize the semaphore set with semctl
    unsigned short semInitVal[] = {0, 0, 0, 0, 0};
    union semun arg;
    arg.array = semInitVal;

    if (semctl(sem_idx_board, 0, SETALL, arg) == -1)
        errExit("semctl SETALL failed");


    //Genera semafori per accesso scacchiera
    // Create a semaphore set with 5 semaphores
    int sem_idx_access = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (sem_idx_access == -1)
        errExit("semget failed");

    union semun arg;
    arg.val = 1;
    if (semctl(sem_idx_access, 1, SETVAL, arg) == -1)
        errExit("semctl SETVAL");


    //Genera semafori per accesso lista di ack
    // Create a semaphore set with 5 semaphores
    int sem_idx_ack = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (sem_idx_ack == -1)
        errExit("semget failed");

    union semun arg;
    arg.val = 1;
    if (semctl(sem_idx_ack, 1, SETVAL, arg) == -1)
        errExit("semctl SETVAL");



    /*---------------------------------------------
        ACK MANAGER
    -----------------------------------------------*/
    //GENERA Ack MANAGER
    pid_t pid_ack_manager = fork();
    if (pid_ack_manager == -1)
        printf("Ack_manager not created!");
    else if (pid_ack_manager == 0) {
        // code executed only by the Ack_manager

        //Genera coda di messaggi
        // get the message queue, or create a new one if it does not exist
        msqid = msgget(msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
        if (msqid == -1)
           errExit("msgget failed");


        while (1) {
            //Controlla i msg
            int msg_id;

            for (int i = 0; i < 100; i++) {
                //Controlla la lista
                if (acknowled_list[i].message_id == message_id) {

                }
            }

            //Invia ack al client

            //Libera memoria

            //Aspetta 5 secondi
            sleep(5);
        }

    }


    pid_t devices_pid[5] = {0}; //Pid devices


    /*---------------------------------------------
        DEVICES
    -----------------------------------------------*/

    // Genera i 5 devices
    for (int dev = 0; dev < 5; ++dev) {
        pid_t pid = fork();
        // check error for fork
        if (pid == -1)
            printf("device %d not created!", dev);
        // check if running process is child or parent
        else if (pid == 0) {
            // code executed only by the child --> Devices

            //Make fifo
            char *baseDeviceFIFO = "/tmp/dev_fifo.";

            // make the path of Device's FIFO
            char path2DeviceFIFO [25];
            sprintf(path2DeviceFIFO, "%s%d", baseDeviceFIFO, getpid());

            if (mkfifo(path2DeviceFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
                errExit("mkfifo failed");

            int deviceFIFO = open(path2DeviceFIFO, O_RDONLY);
            if(deviceFIFO == -1)
                errExit("Open failed");

            // Open an extra descriptor, so that the server does not see end-of-file
            // even if all clients closed the write end of the FIFO
            deviceFIFO_extra = open(path2DeviceFIFO, O_WRONLY);
            if (deviceFIFO_extra == -1)
                errExit("open write-only failed");

            // attach the shared memory segment
            struct Board *board = (struct Board*)get_shared_memory(shmidBoard, 0);

            int devx, devy;

            while (1) {
                //Invio messaggi

                //Ricezione messaggi

                // Movimento
                // wait the i-th semaphore
                semOp(sem_idx_board, (unsigned short)dev, -1);
                    //Accedo alla board
                    //Blocco accesso alla Board
                    semOp(sem_idx_access, 0, -1);

                        updatePosition(file_posizioni);

                    //Sblocco accesso alla board
                    semOp(sem_idx_access, 0, 1);

                //Sblocca il device i+1
                if (dev < 4) {
                    semOp(sem_idx_board, (unsigned short) dev + 1, 1);
                }


            }
        } else {
            //Eseguito da SERVER
            devices_pid[dev] = pid;

        }
    }

    int step = 0;

    while (1) {
        //Stampo le posizioni dei devices e gli id dei msg
        print_device_position();

        //Sblocco il device 1 --> Inizio movimento
        semOp(semid, (unsigned short) 0, 1);

        //Attendo 2 secondi
        sleep(2);
        step++;
    }

    return 0;
}
