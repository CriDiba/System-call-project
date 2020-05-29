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
    pid_t matrix[10][10];
};

/*-----------------------------------------------
    SERVER FUNCTIONS
-----------------------------------------------*/

// Prints devices position and message list
void print_device_position(int step, struct Board *board, Acknowledgment *ack_list, int *devices_pid);

// Scan Board and return in x, y the coordinates of device specified by dev_pid
void get_position(struct Board *board, pid_t dev_pid, int *x, int *y);

// Scan Ack list and print the message_id of every message content in
// msg_list of of device specified by dev_pid
void print_device_msgs(Acknowledgment *ack_list, pid_t dev_pid);

/*-----------------------------------------------
    DEVICES FUNCTIONS
-----------------------------------------------*/

// Reads from file the next position of a device
// and updates it in the board matrix
void updatePosition(struct Board *board, int file_posizioni, int *dev_i, int *dev_j);

// It is a support function that checks if a device in postion (dev_i, dev_j)
// can reach a device in position (other_i, other_j).
int in_range(int dev_i, int dev_j, int other_i, int other_j, double max_dist);

// Checks the ack_list and return 1 if a receiver device
// has already received message with id == message_id, 0 otherwise
int received_yet(Acknowledgment *ack_list, int message_id, pid_t pid_receiver);

// Checks if there are devices in range to send
// all the messages stored in the array of the device
void send_messages(Acknowledgment *ack_list, struct Board *board, int dev_i, int dev_j, Message *msgList, int size);

// Receives all the messages in the device's FIFO and organize the acknowledgement list
void receive_messages(Acknowledgment *ack_list, Message *msgList, int size, int deviceFIFO);

// Updates the acknowledge list with new messages.
// Checks if there are 5 acknowledgements for a specified message
// then delete the message from the array of device that received it last
void update_ack_list(Acknowledgment *ack_list, Acknowledgment ack, Message *msgList, int i);

// Helps to keep the array of messages ordered.
// this method orders the array by message_id in non-ascending order
void sort_msgs(Message *msgList);

/*-----------------------------------------------
    ACK MANAGER FUNCTIONS
-----------------------------------------------*/

// Scan the ack_list and checks if there are 5 messages with
// the same message_id. When it found them, removes messages from
// ack_list and send an acknowledge messages to client.
void check_list(Acknowledgment *ack_list, int msqid);

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
void get_tstamp(time_t timer, char *buffer, size_t buffer_size);
