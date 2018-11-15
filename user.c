#include "oss.h"

int main(int argc, char* argv[]){
	int randomNum = 0, c = 0, MaxUserProcesses = 18;
	randomNum = rand() % (3 + 1 - 0) + 0;

    sharedMemoryConfig();

    //printf("Hello from user.c\n");

    //get the priority of queue
    FILE *fp = fopen("log.txt", "a+");
    switch (randomNum){
        case 1:
            for(c = 0; c < MaxUserProcesses; c++){
                printf("\ncase 1 \n");
                break;
            }
        case 2:
            for(c = 0; c < MaxUserProcesses; c++){
                printf("\ncase 2\n");
                break;
            }
        case 3:
            for(c = 0; c < MaxUserProcesses; c++){
                printf("\ncase 3\n");
                break;
            }
        default:
            printf("Error: Could not add child #%i to a queue!\n");
            fprintf(stderr, "Error adding to queue\n");
            fprintf(fp, "Error: Could not add child #%i to a queue!\n");
            break;
    }
    //do work in critical zone


    shmdt(sysClockshmPtr);//clean
    shmdt(ProcessCtrlBlockshmPtr);//clean
    return 0;
}