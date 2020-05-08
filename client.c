/// @file client.c
/// @brief Contiene l'implementazione del client.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/msg.h>

#include "err_exit.h"
#include "defines.h"

char *baseDeviceFIFO = "/tmp/dev_fifo.";

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

    // create a message data struct
    Message message;

    char buffer[20];
    size_t len;

    // read the receriver's pid
    printf("Inserire pid device a cui inviare il messaggio: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.pid_receiver = readInt(buffer);

    // read the id of the message
    printf("Inserire id messaggio: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.message_id = readInt(buffer);

    // read the text of the message
    printf("Inserire messaggio: ");
    fgets(message.message, sizeof(message.message), stdin);
    len = strlen(message.message);
    message.message[len - 1] = '\0';

    // read the maximum distance of the message
    printf("Inserire massima distanza comunicazione per il messaggio: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.max_distance = readDouble(buffer);

    // get the sender's pid
    message.pid_sender = getpid();

    // make the path of device's FIFO
    char path2DeviceFIFO [25];
    sprintf(path2DeviceFIFO, "%s%d", baseDeviceFIFO, message.pid_receiver);

    // open the device's FIFO to send a message
    int deviceFIFO = open(path2DeviceFIFO, O_WRONLY);
    if(deviceFIFO == -1)
        errExit("Open failed");

    // send the message to the device through the FIFO
    int bW = write(deviceFIFO, &message, sizeof(Message));
    if (bW != sizeof(Message))
        errExit("Write failed");

    // Close the device's FIFO
    if(close(deviceFIFO) == -1)
        errExit("Close failed");

    // get the message queue identifier
    int msqid = msgget(msgKey, S_IRUSR | S_IWUSR);
    if (msqid == -1)
        errExit("msgget failed");

    // create a acknowledge list data struct
    struct ack_list ack_list;

    // read a message from the message queue.
    // type is set equal to client pid.
    // Thus, only the messages with type equals to pid are read.
    size_t mSize = sizeof(struct ack_list) - sizeof(long);
    if (msgrcv(msqid, &ack_list, mSize, getpid(), 0) == -1)
        errExit("Msgget failed");

    // close the standard output fd in order to write on new opened file
    close(STDOUT_FILENO);

    // make the output file path
    char path2MessageOutFile [25];
    sprintf(path2MessageOutFile, "out_%d.txt", message.message_id);

    // create and open the output file for only writing
    int fileOut = open(path2MessageOutFile, O_CREAT | O_EXCL | O_WRONLY, S_IRWXU | S_IRWXG);
    if (fileOut == -1)
        errExit("Open failed");

    // print the acknowledge list on the output file
    printf("Messaggio %d: %s:\n", message.message_id, message.message);
    printf("Lista acknowledgment:\n");
    for (int i = 0; i < 5; i++) {
        printf("%d, %d, %s\n",
            ack_list.acknowledgment[i].pid_sender,
            ack_list.acknowledgment[i].pid_receiver,
            ack_list.acknowledgment[i].timestamp
        );
    }

    // close the file descriptor of the output file
    if(close(fileOut) == -1)
        errExit("close failed");

    return 0;
}
