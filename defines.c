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
