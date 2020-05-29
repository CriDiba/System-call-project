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

/* Global variables */
pid_t devices_pid[5]; // devices pid
pid_t ackManager_pid; // ack Manger pid

int shmidBoard;
int shmidAckList;
Acknowledgment *ack_list;
struct Board *board;

int sem_idx_board;
int sem_idx_access;
int sem_idx_ack;

int msqid;

int deviceFIFO;
char *baseDeviceFIFO = "/tmp/dev_fifo.";
char path2DeviceFIFO [25];


void sigTermHandlerServer(int sig) {

    // send SIGTERM to devices process
    for(int i = 0; i < 5; i++)
        kill(devices_pid[i], SIGTERM);

    // send SIGTERM to ackManager process
    kill(ackManager_pid, SIGTERM);

    // wait for all the children to exit
    while(wait(NULL) != -1);
    if(errno != ECHILD)
        errExit("Unexpected error");

    // remove all semaphores
    if(semctl(sem_idx_board, 0, IPC_RMID, NULL) == -1 ||
        semctl(sem_idx_ack, 0, IPC_RMID, NULL) == -1 ||
        semctl(sem_idx_access, 0, IPC_RMID, NULL) == -1)
        errExit("Could not remove semaphores");

    // remove board shared memory
    free_shared_memory(board);
    remove_shared_memory(shmidBoard);

    // remove acklist shared memory
    free_shared_memory(ack_list);
    remove_shared_memory(shmidAckList);

    // terminate the server process
    exit(0);
}

void sigTermHandlerAckManager(int sig) {

    // remove message queue
    if(msgctl(msqid, IPC_RMID, NULL) == -1)
        errExit("Could not remove message queue");

    // terminate the ack manager process
    exit(0);
}

void sigTermHandlerDevice(int sig) {

    // close device FIFO
    if (close(deviceFIFO) == -1)
        errExit("close failed");

    // remove device FIFO
    if (unlink(path2DeviceFIFO) != 0)
        errExit("unlink failed");

    // terminate the device process
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

    // set the function sigTermHandlerServer as handler for the signal SIGTERM
    if(signal(SIGTERM, sigTermHandlerServer) == SIG_ERR)
        errExit("change signal handler failed");

    /*-----------------------------------------------
        SHARED MEMORY
    -----------------------------------------------*/

    // allocate and attach the shared memory segment for Board
    shmidBoard = alloc_shared_memory(IPC_PRIVATE, sizeof(struct Board));
    board = (struct Board *)get_shared_memory(shmidBoard, 0);

    // allocate and attach the shared memory segment for Acknowledge List
    shmidAckList = alloc_shared_memory(IPC_PRIVATE, 100*sizeof(Acknowledgment));
    ack_list = (Acknowledgment *)get_shared_memory(shmidAckList, 0);


    /*-----------------------------------------------
        SEMAPHORES
    -----------------------------------------------*/

    // create a semaphore set with 5 semaphores for Devices'movement
    sem_idx_board = semget(IPC_PRIVATE, 5, S_IRUSR | S_IWUSR);
    if (sem_idx_board == -1)
        errExit("semget failed");

    // initialize the semaphore set with semctl
    unsigned short semInitVal[] = {0, 0, 0, 0, 0};
    union semun arg;
    arg.array = semInitVal;

    if (semctl(sem_idx_board, 0, SETALL, arg) == -1)
        errExit("semctl SETALL failed");

    // Create a semaphore for Board access
    sem_idx_access = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (sem_idx_access == -1)
        errExit("semget failed");

    arg.val = 5;
    if (semctl(sem_idx_access, 0, SETVAL, arg) == -1)
        errExit("semctl SETVAL");

    // create a semaphore for Acknowledge List access
    sem_idx_ack = semget(IPC_PRIVATE, 1, S_IRUSR | S_IWUSR);
    if (sem_idx_ack == -1)
        errExit("semget failed");

    arg.val = 1;
    if (semctl(sem_idx_ack, 0, SETVAL, arg) == -1)
        errExit("semctl SETVAL");


    /*-----------------------------------------------
        ACK MANAGER
    -----------------------------------------------*/

    // create the Acknowledge Manager process child
    ackManager_pid = fork();
    if (ackManager_pid == -1)
        printf("Ack_manager not created!");
    else if (ackManager_pid == 0) {
        // code executed only by the Ack_manager

        // set the function sigTermHandlerAckManager as handler for the signal SIGTERM
        if(signal(SIGTERM, sigTermHandlerAckManager) == SIG_ERR)
            errExit("change signal handler failed");

        // get the message queue, or create a new one if it does not exist
        msqid = msgget(msgKey, IPC_CREAT | S_IRUSR | S_IWUSR);
        if (msqid == -1)
           errExit("msgget failed");

        while (1) {
            // checks messages in Ack List
            semOp(sem_idx_ack, 0, -1);
            check_list(ack_list, msqid);
            semOp(sem_idx_ack, 0, 1);

            // wait 5 seconds
            sleep(5);
        }
    }


    /*-----------------------------------------------
        DEVICES
    -----------------------------------------------*/

    // create 5 device process children
    for (int dev = 0; dev < 5; ++dev) {
        devices_pid[dev] = fork();
        if (devices_pid[dev] == -1)
            printf("device %d not created!", dev);
        else if (devices_pid[dev] == 0) {
            // code executed only by the Device

            // set the function sigTermHandlerDevice as handler for the signal SIGTERM
            if(signal(SIGTERM, sigTermHandlerDevice) == SIG_ERR)
                errExit("signal failed");

            // make the path of device's FIFO
            sprintf(path2DeviceFIFO, "%s%d", baseDeviceFIFO, getpid());

            // make device's FIFO
            if (mkfifo(path2DeviceFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1)
                errExit("mkfifo failed");

            // Open the FIFO in non-blocking mode
            deviceFIFO = open(path2DeviceFIFO, O_RDONLY | O_NONBLOCK);
            if(deviceFIFO == -1)
                errExit("Open failed");

            int dev_i = -1, dev_j = -1;
            // set device position on Board
            semOp(sem_idx_board, dev, -1);
            updatePosition(board, file_posizioni, &dev_i, &dev_j);
            if (dev < 4)
                semOp(sem_idx_board, (unsigned short) dev + 1, 1);

            // when all devices join the board unlock the server
            semOp(sem_idx_access, 0, -1);

            // received messages list
            Message msgList[20] = {0};

            while (1) {
                // send messages to other devices
                semOp(sem_idx_ack, 0, -1);
                send_messages(ack_list, board, dev_i, dev_j, msgList, 20);
                semOp(sem_idx_ack, 0, 1);

                // order the list of messages and pushing to the tail
                // the "non-messages" (message_id = 0)
                sort_msgs(msgList);

                // receive messages to other devices or clients
                semOp(sem_idx_ack, 0, -1);
                receive_messages(ack_list, msgList, 20, deviceFIFO);
                semOp(sem_idx_ack, 0, 1);

                // update device position on Board
                semOp(sem_idx_board, dev, -1);
                updatePosition(board, file_posizioni, &dev_i, &dev_j);
                if (dev < 4)
                    semOp(sem_idx_board, dev + 1, 1);

                // when all devices update position unlock the server
                semOp(sem_idx_access, 0, -1);
            }
        }
    }


    /*-----------------------------------------------
        SERVER
    -----------------------------------------------*/

    int step = 0;
    while (1) {
        // unlock the first device
        semOp(sem_idx_board, 0, 1);
        // wait for all the devices to update position on Board
        // and print devices positions and message info
        semOp(sem_idx_access, 0, 0);
        semOp(sem_idx_ack, 0, -1);
        print_device_position(step, board, ack_list, devices_pid);
        semOp(sem_idx_ack, 0, 1);
        semOp(sem_idx_access, 0, 5);

        // wait 2 seconds
        sleep(2);
        step++;
    }

    return 0;
}
