/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

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
