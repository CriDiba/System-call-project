/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <string.h>
#include <stdio.h>
#include <sys/msg.h>

#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"


/*-----------------------------------------------
    DATA STRUCTURES
-----------------------------------------------*/

typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    char message[256];
    int max_distance;
} Message;


typedef struct {
    pid_t pid_sender;
    pid_t pid_receiver;
    int message_id;
    time_t timestamp;
} Acknowledgment;


struct ack_message {
    long mtype;
    Acknowledgment acknowledgment [5];
};

struct Board {
    int matrix[10][10];
};


/*-----------------------------------------------
    SERVER FUNCTIONS
-----------------------------------------------*/
void print_device_position(int step, struct Board * board, Acknowledgment * ack_list, int * devices_pid);
int get_i(struct Board * board, pid_t dev_pid);
int get_j(struct Board * board, pid_t dev_pid);
void print_device_msgs(Acknowledgment * ack_list, pid_t dev_pid);

/*-----------------------------------------------
    DEVICES FUNCTIONS
-----------------------------------------------*/
void initialize(Message * msgList, int size);

void updatePosition(struct Board * board, int file_posizioni, int *dev_i, int *dev_j);

int in_range(int dev_i, int dev_j, int other_i, int other_j, double max_dist);

int received_yet(Acknowledgment * ack_list, int message_id, pid_t pid_receiver);

void send_messages(Acknowledgment * ack_list, struct Board * board, int dev_i, int dev_j, Message * deviceMsgList, int size);

void receive_messages(Acknowledgment * ack_list, Message * deviceMsgList, int size, int sem_idx_ack, int deviceFIFO);

void update_ack_list(Acknowledgment * ack_list, Acknowledgment ack, int sem_idx_ack);

/*-----------------------------------------------
    ACK MANAGER FUNCTIONS
-----------------------------------------------*/

void check_list(Acknowledgment * ack_list, int msqid);


/*-----------------------------------------------
    CLIENTS FUNCTIONS
-----------------------------------------------*/

// The readInt reads a integer value from an array of chars
// It checks that only a number n is provided as an input parameter,
// and that n is greater than 0
int readInt(const char *s);

// The readDouble reads a double value from an array of chars
// It checks that only a number n is provided as an input parameter,
// and that n is greater than 0
double readDouble(const char *s);

// Returns in buffer a string with timestamp
// in format YYYY-MM-DD hh:mm:ss
void get_tstamp(time_t timer, char * buffer, size_t buffer_size);
