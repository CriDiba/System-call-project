#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

char *path2OutFile = "file_posizioni.txt";

int is_taken(int dev_position[5][2], int dev, int dev_i, int dev_j) {
    for (int i = 0; i < 5; i++)
        if (i != dev)
            if (dev_position[i][0] == dev_i && dev_position[i][1] == dev_j)
                return 1;
    return 0;
}

int main(int argc, char * argv[]) {

    if (argc != 2) {
        printf("Usage: %s step\n", argv[0]);
        exit(1);
    }

    int step = atoi(argv[1]);
    if (step <= 0) {
        printf("The number of step must be greater than zero!\n");
        exit(1);
    }

    srand(time(NULL));
    int dev_position[5][2] = {{-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}, {-1,-1}};

    close(STDOUT_FILENO);
    open(path2OutFile, O_TRUNC | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

    for (int i = 0; i < step; i++) {
        for (int dev = 0; dev < 5; dev++) {
            int dev_i, dev_j;
            if (dev_position[dev][0] == -1 && dev_position[dev][1] == -1) {
                //first step
                do {
                    dev_i = rand() % 10;
                    dev_j = rand() % 10;
                } while(is_taken(dev_position, dev, dev_i, dev_j));
                dev_position[dev][0] = dev_i;
                dev_position[dev][1] = dev_j;
            } else {
                //other step
                int max_i = 3, min_i = 1, max_j = 3, min_j = 1;
                if (dev_position[dev][0] == 0) {
                    max_i = 2; min_i = 0;
                } else if(dev_position[dev][0] == 9)
                    max_i = 2;

                if (dev_position[dev][1] == 0) {
                    max_j = 2; min_j = 0;
                } else if(dev_position[dev][1] == 9)
                    max_j = 2;

                do {
                    dev_i = dev_position[dev][0] + ((rand() % max_i) - min_i);
                    dev_j = dev_position[dev][1] + ((rand() % max_j) - min_j);
                } while(is_taken(dev_position, dev, dev_i, dev_j));
                dev_position[dev][0] = dev_i;
                dev_position[dev][1] = dev_j;
            }

            if (dev < 4)
                printf("%d,%d|", dev_position[dev][0], dev_position[dev][1]);
            else
                printf("%d,%d\n", dev_position[dev][0], dev_position[dev][1]);
        }
    }

}
