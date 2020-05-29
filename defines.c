/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"

/*-----------------------------------------------
    SERVER FUNCTIONS
-----------------------------------------------*/

void print_device_position(int step, struct Board * board, Acknowledgment * ack_list, int * devices_pid) {
    printf("# Step %2d: device positions #################\n", step);

    int dev_i, dev_j;
    for (int dev = 0; dev < 5; dev++) {
        get_position(board, devices_pid[dev], &dev_i, &dev_j);
        printf("%d %d %d msgs: ", devices_pid[dev], dev_i, dev_j);
        print_device_msgs(ack_list, devices_pid[dev]);
        printf("\n");
    }

    printf("#############################################\n");
}

void get_position(struct Board * board, pid_t dev_pid, int *x, int *y) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (board->matrix[i][j] == dev_pid) {
                *x = i;
                *y = j;
                return;
            }
        }
    }
}

void print_device_msgs(Acknowledgment *ack_list, pid_t dev_pid) {
    for (int i = 0; i < 100; i++) {
        if (ack_list[i].pid_receiver == dev_pid && ack_list[i + 1].message_id == 0) {
            printf("%d ", ack_list[i].message_id);
        }
    }
}

/*-----------------------------------------------
    DEVICES FUNCTIONS
-----------------------------------------------*/

void updatePosition(struct Board *board, int file_posizioni, int *dev_i, int *dev_j) {
    //read the first 3 characters of the file
    char buffer[4];
    int bR = read(file_posizioni, buffer, 3);

    //buffer now should contain a string
    //with the format: "x,y"
    if (bR == -1)
        errExit("read failed");
    else if (bR != 0) {
        buffer[bR] = '\0';
        //lseek skips the next '|' or '\n'
        lseek(file_posizioni, 1, SEEK_CUR);

        int i = (int)(buffer[0] - '0');
        int j = (int)(buffer[2] - '0');

        if(board->matrix[i][j] != 0 && board->matrix[i][j] != getpid()){
            printf("Error, cell is not free!\n");
            exit(1);
        }

        if (*dev_i != -1 && *dev_j != -1)
            board->matrix[*dev_i][*dev_j] = 0;

        board->matrix[i][j] = getpid();
        *dev_i = i;
        *dev_j = j;
    }
}

int in_range(int dev_i, int dev_j, int other_i, int other_j, double max_dist) {
    int diff_i = dev_i - other_i;
    int diff_j = dev_j - other_j;
    return ((diff_i * diff_i) + (diff_j * diff_j) <= (max_dist * max_dist));
}

int received_yet(Acknowledgment * ackList, int message_id, pid_t pid_receiver) {
    for (int i = 0; i < 100; i++) {
        if (ackList[i].message_id == message_id && ackList[i].pid_receiver == pid_receiver) {
            return 1;
        }
    }
    return 0;
}

void send_messages(Acknowledgment *ack_list, struct Board *board, int dev_i, int dev_j, Message *msgList, int size) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {

            if (board->matrix[i][j] != 0 && (i != dev_i || j != dev_j)) {
                pid_t pid_other_dev = board->matrix[i][j];
                int send = 0;

                //Make fifo
                char *baseDeviceFIFO = "/tmp/dev_fifo.";
                // make the path of other Device's FIFO
                char path2DeviceFIFO [25];
                sprintf(path2DeviceFIFO, "%s%d", baseDeviceFIFO, pid_other_dev);
                // open the other device FIFO
                int oherDeviceFIFO = open(path2DeviceFIFO, O_WRONLY);
                if (oherDeviceFIFO == -1)
                    errExit("open write-only failed");

                //
                for (int msg = 0; msg < size && msgList[msg].message_id != 0; msg++) {
                    if (in_range(dev_i, dev_j, i, j, msgList[msg].max_distance) &&
                        !received_yet(ack_list, msgList[msg].message_id, pid_other_dev)) {

                        // update sender and receiver pid
                        msgList[msg].pid_sender = getpid();
                        msgList[msg].pid_receiver = pid_other_dev;

                        int wB = write(oherDeviceFIFO, &msgList[msg], sizeof(Message));
                        if (wB != sizeof(Message))
                            errExit("Write failed");

                        send++;
                        // mark message as sent
                        msgList[msg].message_id = 0;
                    }
                }

                if(close(oherDeviceFIFO) == -1)
                    errExit("Close failed");
                //return because we can send messages to a single device
                if (send)
                    return;
            }

        }
    }
}

void receive_messages(Acknowledgment *ack_list, Message *msgList, int size, int deviceFIFO) {
    // find the first free position of the deviceMsgList array
    int i;
    for (i = 0; i < size && msgList[i].message_id != 0; i++);

    //if there is a free position then read from fifo
    if (i != size) {
        //while there is something to read and the last position
        //of the array is not reached, read from fifo
        int bR;
        do {
            bR = read(deviceFIFO, &msgList[i], sizeof(Message));

            if (bR == -1)
                printf("It looks like the FIFO is broken");
            else if (bR > 0) {
                Acknowledgment ack = {
                    msgList[i].pid_sender,
                    msgList[i].pid_receiver,
                    msgList[i].message_id,
                    time(NULL)
                };

                update_ack_list(ack_list, ack, msgList, i);
                i++;
            }
        } while(bR > 0 && i < size);
    }
}

void update_ack_list(Acknowledgment *ack_list, Acknowledgment ack, Message *msgList, int i) {
    int index_first_free_area = -1;
    //at the end of this for loop found:
    //  - the index of the first free area of the list if this is the first ack with that message_id
    //  - the index of the first occurence of message_id in the list
    //  - the list is full
    int j;
    for(j = 0; j < 100 && ack_list[j].message_id != ack.message_id; j += 5){
        if(index_first_free_area == -1 && ack_list[j].message_id == 0)
            index_first_free_area = j;
    }

    if(j != 100){
        // how many ack have the same message_id
        int find_ack = 0;
        while(ack_list[j].message_id == ack.message_id) {
            j++;
            find_ack++;
        }
        ack_list[j] = ack;
        find_ack++;

        //if there are 5 acknowledgements
        //then delete the message from the array
        if(find_ack == 5)
            msgList[i].message_id = 0;
    }
    else if(index_first_free_area != -1)
        ack_list[index_first_free_area] = ack;
    else
        // if the list is full then the message is rejected
        exit(1);
}

void sort_msgs(Message *msgList) {
    // implementation of insertion sort:
    // sorting on message id's
    for(int j = 1; j < 20; j++){
        Message tmp = msgList[j];
        int i = j - 1;
        while(i >= 0 && msgList[i].message_id < tmp.message_id){
            msgList[i+1] = msgList[i];
            i--;
        }
        msgList[i+1] = tmp;
    }
}

/*-----------------------------------------------
    ACK MANAGER FUNCTIONS
-----------------------------------------------*/

void check_list(Acknowledgment *ack_list, int msqid) {
    for (int i = 0; i < 100; i += 5) {
        int message_id = ack_list[i].message_id;

        if (message_id != 0) {
            // how many ack have the same message_id
            int find_ack = 1;
            for (int j = i + 1; j < i + 5; j++) {
                if (ack_list[j].message_id == message_id) {
                    find_ack++;
                }
            }

            if (find_ack != 5)
                continue;

            //create the message for the message queue;
            struct ack_message clientMessage;
            clientMessage.mtype = ack_list[i].pid_sender;

            for (int j = i; j < i + 5; j++) {
                clientMessage.acknowledgment[j - i] = ack_list[j];
                //mark the position as free
                ack_list[j].message_id = 0;
                ack_list[j].pid_receiver = -1;
                ack_list[j].pid_sender = -1;
            }

            // send the message to the client through the message queue
            size_t mSize = sizeof(struct ack_message) - sizeof(long);
            if (msgsnd(msqid, &clientMessage, mSize, 0) == -1)
                errExit("msgsnd failed");
        }
    }
}


/*-----------------------------------------------
    CLIENTS FUNCTIONS
-----------------------------------------------*/

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

double readDouble(const char *s) {
    char *endptr = NULL;
    errno = 0;
    double res = strtod(s, &endptr);
    if (errno != 0 || *endptr != '\n' || res < 0) {
        printf("invalid input argument\n");
        exit(1);
    }

    return res;
}

void get_tstamp(time_t timer, char *buffer, size_t buffer_size){
    struct tm * tm_info = localtime(&timer);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
}
