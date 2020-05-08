/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

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


struct ack_list {
    long mtype;
    Acknowledgment acknowledgment [5];
};


/*-----------------------------------------------
    SERVER FUNCTIONS
-----------------------------------------------*/

// Returns in buffer a string with timestamp
// in format YYYY-MM-DD hh:mm:ss
void get_tstamp(time_t timer, char * buffer, size_t buffer_size);


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
