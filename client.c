/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "err_exit.h"
#include "defines.h"

char *baseDeviceFIFO = "/tmp/dev_fifo.";
char * msg_id_history_file = "/tmp/msg_id_history_file";

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

    // open history file to check if the message id is taken
    int history_fd = open(msg_id_history_file, O_RDWR);
    if(history_fd == -1) {
        perror("<Client> Could not open history file!");
        printf("<Client> Server might be down, terminating session...\n");
        exit(1);
    }

    // create a message data struct
    Message message;

    char buffer[20];
    size_t len;

    printf("<Client> Hello, let's send a message!\n");

    // read the receriver's pid
    printf("<Client> Insert the receiver's pid: ");
    fgets(buffer, sizeof(buffer), stdin);
    message.pid_receiver = readInt(buffer);

    // read the id of the message
    do {
        printf("<Client> Insert the message id: ");
        fgets(buffer, sizeof(buffer), stdin);
        message.message_id = readInt(buffer);

        //Check validity of message_id
        if(message.message_id <= 0){
            printf("<Client> Message id must be greater than 0!\n");
            exit(1);
        }
    } while(! msg_id_available(history_fd, message.message_id));

    // read the text of the message
    printf("<Client> Insert your message: ");
    fgets(message.message, sizeof(message.message), stdin);
    len = strlen(message.message);
    message.message[len - 1] = '\0';

    // read the maximum distance of the message
    printf("<Client> Insert the max distance: ");
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
        errExit("<Client> Open failed");

    // send the message to the device through the FIFO
    int bW = write(deviceFIFO, &message, sizeof(Message));
    if (bW != sizeof(Message))
        errExit("<Client> Write failed");

    // Close the device's FIFO
    if(close(deviceFIFO) == -1)
        errExit("<Client> Close failed");

    printf("<Client> Message sent successfully!\n");
    printf("<Client> Waiting for a response...\n");

    // get the message queue identifier
    int msqid = msgget(msgKey, S_IRUSR | S_IWUSR);
    if (msqid == -1)
        errExit("<Client> Message queues does not exists");

    // create an acknowledge message data struct
    struct ack_message ack_list;

    // read a message from the message queue.
    // type is set equal to client pid.
    // Thus, only the messages with type equals to pid are read.
    size_t mSize = sizeof(struct ack_message) - sizeof(long);
    if (msgrcv(msqid, &ack_list, mSize, getpid(), 0) == -1)
        errExit("<Client> Msgget failed");

    // close the standard output fd in order to write on new opened file
    int dup_stdout = dup(STDOUT_FILENO);
    close(STDOUT_FILENO);

    // make the output file path
    char path2MessageOutFile [25];
    sprintf(path2MessageOutFile, "out_%d.txt", message.message_id);

    // create and open the output file for only writing
    int fileOut = open(path2MessageOutFile, O_CREAT | O_EXCL | O_WRONLY, S_IRWXU | S_IRWXG);
    if (fileOut == -1)
        errExit("<Client> Open failed");

    // print the acknowledge list on the output file
    printf("Messaggio %d: %s\n", message.message_id, message.message);
    printf("Lista acknowledgment:\n");
    for (int i = 0; i < 5; i++) {
        char timestamp[20];
        get_tstamp(ack_list.acknowledgment[i].timestamp, timestamp, 20);

        printf("%d, %d, %s\n",
            ack_list.acknowledgment[i].pid_sender,
            ack_list.acknowledgment[i].pid_receiver,
            timestamp
        );
    }

    // close the file descriptor of the output file
    if(close(fileOut) == -1)
        errExit("<Client> Close failed");

    // close the file descriptor of the history file
    if(close(history_fd) == -1)
        errExit("<Client> Close failed");

    // print final message on dup stdout
    char result[100];
    sprintf(result, "<Client> Acknowledge list printed in the %s file!\n", path2MessageOutFile);
    if(write(dup_stdout, result, strlen(result)) == -1)
        errExit("<Client> Write failed");

    return 0;
}
