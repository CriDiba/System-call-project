/// @file server.c
/// @brief Contiene l'implementazione del SERVER.
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"

/*Global variables*/
pid_t devices_pid[5]; //Devices pid
pid_t ackManager_pid; //Ack Manger pid

int shmidBoard;
int shmidAckList;
Acknowledgment * ack_list;
struct Board * board;

int sem_idx_board;
int sem_idx_access;
int sem_idx_ack;

int msqid;

int deviceFIFO;
char path2DeviceFIFO [25];


void sigTermHandlerServer(int sig) {

    //Kill devices figli
    for(int i = 0; i < 5; i++)
        kill(devices_pid[i], SIGTERM);

    //Kill ack manager
    kill(ackManager_pid, SIGTERM);

    //wait for all the children to exit
    while(wait(NULL) != -1);
    if(errno != ECHILD)
        errExit("Unexpected error");

    //remove semaphores
    if(semctl(sem_idx_board, 0, IPC_RMID, NULL) == -1 ||
        semctl(sem_idx_ack, 0, IPC_RMID, NULL) == -1 ||
        semctl(sem_idx_access, 0, IPC_RMID, NULL) == -1)
        errExit("could not remove semaphores");

    //remove board
    free_shared_memory(board);
    remove_shared_memory(shmidBoard);

    //remove acklist
    free_shared_memory(ack_list);
    remove_shared_memory(shmidAckList);

    // terminate the server process
    exit(0);
}

void sigTermHandlerAckManager(int sig) {

    //Rimuovo IPC
    if(msgctl(msqid, IPC_RMID, NULL) == -1)
        errExit("msgctl IPC_RMID failed");

    // terminate the ack manager process
    exit(0);
}

void sigTermHandlerDevice(int sig) {

    //Rimuovo IPC
    // Close the FIFO
    if (close(deviceFIFO) == -1)
        errExit("close failed");

    // Remove the FIFO
    if (unlink(path2DeviceFIFO) != 0)
        errExit("unlink failed");

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
    if(signal(SIGTERM, sigTermHandlerServer) == SIG_ERR)
        errExit("change signal handler failed");

    /*-----------------------------------------------
        SHARED MEMORY
    -----------------------------------------------*/

    //GENERA MEMEORIA CONDIVISA Scacchiera
    shmidBoard = alloc_shared_memory(IPC_PRIVATE, sizeof(struct Board));
    board = (struct Board *)get_shared_memory(shmidBoard, 0);

    //Genera memoria CONDIVISA ACK
    shmidAckList = alloc_shared_memory(IPC_PRIVATE, 100*sizeof(Acknowledgment));
    ack_list = (Acknowledgment*)get_shared_memory(shmidAckList, 0);


    /*-----------------------------------------------
        SEMAPHORES
    -----------------------------------------------*/

    // Genera semafori per movimento
    // Create a semaphore set with 5 semaphores
    sem_idx_board = semget(IPC_PRIVATE, 5, S_IRUSR | S_IWUSR);
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
    sem_idx_access = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (sem_idx_access == -1)
        errExit("semget failed");

    arg.val = 1;
    if (semctl(sem_idx_access, 0, SETVAL, arg) == -1)
        errExit("semctl SETVAL");


    //Genera semafori per accesso lista di ack
    // Create a semaphore set with 5 semaphores
    sem_idx_ack = semget(IPC_PRIVATE, 0, S_IRUSR | S_IWUSR);
    if (sem_idx_ack == -1)
        errExit("semget failed");

    arg.val = 1;
    if (semctl(sem_idx_ack, 1, SETVAL, arg) == -1)
        errExit("semctl SETVAL");



    /*-----------------------------------------------
        ACK MANAGER
    -----------------------------------------------*/
    //GENERA Ack MANAGER
    ackManager_pid = fork();
    if (ackManager_pid == -1)
        printf("Ack_manager not created!");
    else if (ackManager_pid == 0) {
        // code executed only by the Ack_manager

        // set the function sigHandler as handler for the signal SIGTERM
        if(signal(SIGTERM, sigTermHandlerAckManager) == SIG_ERR)
            errExit("change signal handler failed");

        //Genera coda di messaggi
        // get the message queue, or create a new one if it does not exist
        msqid = msgget(msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
        if (msqid == -1)
           errExit("msgget failed");


        while (1) {
            //Controlla i msg
            semOp(sem_idx_ack, 0, -1);

            check_list(ack_list, msqid);

            semOp(sem_idx_ack, 0, 1);

            //Aspetta 5 secondi
            sleep(5);
        }
    }


    /*-----------------------------------------------
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

            if(signal(SIGTERM, sigTermHandlerDevice) == SIG_ERR)
                errExit("signal failed");

            //Make fifo
            char *baseDeviceFIFO = "/tmp/dev_fifo.";

            // make the path of Device's FIFO
            //char path2DeviceFIFO [25];
            sprintf(path2DeviceFIFO, "%s%d", baseDeviceFIFO, getpid());

            if (mkfifo(path2DeviceFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
                errExit("mkfifo failed");

            //Open NON BLOCCANTE
            deviceFIFO = open(path2DeviceFIFO, O_RDONLY | O_NONBLOCK);
            if(deviceFIFO == -1)
                errExit("Open failed");

            // Open an extra descriptor, so that the server does not see end-of-file
            // even if all clients closed the write end of the FIFO
            int deviceFIFO_extra = open(path2DeviceFIFO, O_WRONLY);
            if (deviceFIFO_extra == -1)
                errExit("open write-only failed");

            int dev_i = -1, dev_j = -1;

            //Posizionamento iniziale
            semOp(sem_idx_board, dev, -1);

            updatePosition(board, file_posizioni, &dev_i, &dev_j);

            if (dev < 4)
                semOp(sem_idx_board, (unsigned short) dev + 1, 1);

            //when all devices join the board
            //this semaphore will unlock the server
            //so it can print positions
            semOp(sem_idx_access, 0, -1);

            //Lista dei messaggi ricevuti
            Message msgList[20];
            //init the list
            initialize(msgList, 20); //??????

            while (1) {
                //Invio messaggi
                //Blocca board
                semOp(sem_idx_ack, 0, -1);
                send_messages(ack_list, board, dev_i, dev_j, msgList, 20);
                semOp(sem_idx_ack, 0, 1);
                //Sblocca board

                //Ricezione messaggi
                semOp(sem_idx_ack, 0, -1);
                receive_messages(ack_list, msgList, 20, sem_idx_ack, deviceFIFO);
                semOp(sem_idx_ack, 0, 1);

                // Movimento
                // wait the i-th semaphore
                semOp(sem_idx_board, dev, -1);
                updatePosition(board, file_posizioni, &dev_i, &dev_j);
                //Sblocca il device i+1
                if (dev < 4)
                    semOp(sem_idx_board, dev + 1, 1);

                //Sblocco accesso alla board
                semOp(sem_idx_access, 0, -1);
            }
        } else {
            //Eseguito da SERVER
            devices_pid[dev] = pid;
        }
    }


    /*-----------------------------------------------
        SERVER
    -----------------------------------------------*/
    int step = 0;
    while (1) {
        //Unlocks the first device
        semOp(sem_idx_board, 0, 1);

        //Waits for all the devices to updatePosition()
        //and prints the positions
        semOp(sem_idx_access, 0, 0);
        //Stampo le posizioni dei devices e gli id dei msg
        print_device_position(step, board, ack_list, devices_pid);
        //Set again the semaphore to N_DEVICES
        semOp(sem_idx_access, 0, 5);

        //Attendo 2 secondi
        sleep(2);
        step++;
    }

    return 0;
}
