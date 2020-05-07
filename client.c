/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/msg.h>

#include "err_exit.h"
#include "defines.h"

int readInt(const char *s) {
    char *endptr = NULL;

    errno = 0;
    long int res = strtol(s, &endptr, 10);
    if (errno != 0 || *endptr != '\n' || res < 0) {
        printf("invalid input argument\n");
        exit(1);
    }

    return res;
}

int main(int argc, char * argv[]) {

    // check command line input arguments
    if (argc != 2) {
        printf("Usage: %s msg_queue_key\n", argv[0]);
        exit(1);
    }

    // read the message queue key defined by user
    int msgKey = atoi(argv[1]);
    if (msgKey <= 0) {
        printf("The message queue key must be greater than zero!\n");
        exit(1);
    }

    // crea an message data struct
    Message message;

    char buffer[10];
    size_t len;

    // PID pid_receiver
    printf("Inserire pid device a cui inviare il messaggio: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.pid_receiver = readInt(buffer);

    // PID pid_receiver
    printf("Inserire id messaggio: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.message_id = readInt(buffer);

    // read a description of the order
    printf("Inserire messaggio: ");
    fgets(message.message, sizeof(message.message), stdin);
    len = strlen(message.message);
    message.message[len - 1] = '\0';

    // PID pid_receiver
    printf("Inserire massima distanza comunicazione per il messaggio: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.code = readInt(buffer);

    message.pid_sender = getpid();


    //OPEN FIFO
    char *baseDeviceFIFO = "/tmp/dev_fifo.";

    // make the path of Device's FIFO
    char path2DeviceFIFO [25];
    sprintf(path2DeviceFIFO, "%s%d", baseDeviceFIFO, message.pid_receiver);

    int deviceFIFO = open(path2DeviceFIFO, O_WRONLY);
    if(deviceFIFO == -1)
        errExit("Open failed");

    // INVIA IL MESSAGGIO ATTRAVERSO LA FIFO
    int bW = write(deviceFIFO, &message, sizeof(Message));
    if (bW != sizeof(Message))
        errExit("Write failed");

    if(close(deviceFIFO) == -1)
      errExit("Close failed");


    //ATTENDE LA MSG queue

    Acknowledgment acknowledgment;

    // get the message queue identifier
    int msqid = msgget(msgKey, S_IRUSR | S_IWUSR);
    if (msqid == -1)
        errExit("msgget failed");


    struct ack_list ack_list;
    //Leggo dalla coda il primo msg con mtype = al pid
    size_t mSize = sizeof(struct ack_list) - sizeof(long);
    if (msgrcv(msqid, &ack_list, mSize, getpid(), 0) == -1)
      errExit("msgget failed");


    //SCRIVO SU FILE
    // close the standard output and error stream
    close(STDOUT_FILENO);

    // make the path file
    char path2MessageOutFile [25];
    sprintf(path2MessageOutFile, "out_%d.txt", message.message_id);

    // open the source file for only writing
    int fileOut = open(path2MessageOutFile, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fileOut == -1)
        errExit("open failed");

    // Print on the FD 1 --> FILE
    printf("Messaggio %d: %s:\n", message.message_id, message.message);
    printf("Lista acknowledgment:\n");
    //Per ogni elemento della lista di Ack
    for (int i = 0; i < 5; i++) {
        printf("%d, %d, %s\n", ack_list.acknowledgment[i].pid_sender, ack_list.acknowledgment[i].pid_receiver, ack_list.acknowledgment[i].timestamp);
    }


    // close the file descriptor of the empty file
    close(fileOut);

    return 0;
}
