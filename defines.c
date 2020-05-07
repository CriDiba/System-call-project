/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"



void updatePosition(int pos_file_fd){
    char buffer[4];
    int bR = read(pos_file_fd, buffer, 3);
    if(bR != -1){
        buffer[bR] = '\0';
        lseek(pos_file_fd, 1, SEEK_CUR);
    }
    else
        errExit("read failed");

    int x = (int)(buffer[0] - '0');
    int y = (int)(buffer[2] - '0');

    if(board->matrix[x][y] != 0){
        printf("Error, cell is not free!\n")
        kill(getppid(), SIGTERM);
    }

    board->matrix[x][y] = getpid();
}

void print_device_position() {
    printf("# Step %d: device positions ########################\n", step);

    for (int i = 0; i < 5; i++) {
        printf("%d %d %d msgs: ", devices_pid[i], getX(devices_pid[i]), getY(devices_pid[i]));
        print_device_msgs();
        printf("\n");
    }

    printf("#############################################\n");
}

int getX(pid_t pid) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (board[i][j] == pid) {
                return i;
            }
        }
    }
}

int getY(pid_t pid) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (board[i][j] == pid) {
                return j;
            }
        }
    }
}

void print_device_msgs() {
    //Stampa lista messaggi
}
