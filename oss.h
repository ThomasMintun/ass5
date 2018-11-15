#ifndef OSS_H
#define OSS_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

//shared mem keys, probs not secure in a header
#define CLOCK_SHMKEY 696969
#define PCB_SHMKEY 969696

//struct for rescource descriptor
typedef struct{
    // for managing pids, job of process, and time request iintervals
    pid_t pids[18];
    int pidJob[18];
    int nanosRequest[18];

    // tables for safety alg (banker's alg)
    int resources[20];
    int max[18][20];
    int allocated[18][20];
    int request[18][20];
} resourceS;

resourceS* RDPtr;

typedef struct {
    pid_t pidHolder;

    int startCPUSeconds;
    int startCPUNanoS;

    int timeWorkingSeconds;
    int timeWorkingNanoS;

    int totalCPUSeconds;
    int totalCPUNanoS;

    int burstValSeconds;
    int burstValNanoS;

    int complete;
    int priotiyValue;
    int bitVector;
} PCB_t[18];

typedef struct systemClock_t{
    unsigned int seconds;
    unsigned int nanoseconds;
} systemClock_t;//name systemClock_t was down here

// globals for accessing pointers to shared memory
int sysClockshmid;//holds the shared memory segment id
int ProcessCtrlBlockshmID;
systemClock_t *sysClockshmPtr;//points to the data structure
PCB_t *ProcessCtrlBlockshmPtr;//points to the data structure
int q = 6969;//time quantun

//allocates shared mem
void sharedMemoryConfig(){

    //shared mem for sysClock
    sysClockshmid = shmget(CLOCK_SHMKEY, sizeof(systemClock_t), IPC_CREAT|0777);
    if(sysClockshmid < 0){
        perror("sysClock shmget error in master \n");
        exit(errno);
    }
    sysClockshmPtr = shmat(sysClockshmid, NULL, 0);
    if(sysClockshmPtr < 0){
        perror("sysClock shmat error in oss\n");
        exit(errno);
    }

    //shared mem for PCB
    ProcessCtrlBlockshmID = shmget(PCB_SHMKEY, sizeof(PCB_t), IPC_CREAT|0777);
    if(ProcessCtrlBlockshmID < 0){
        perror("PCB shmget error in master \n");
        exit(errno);
    }
    ProcessCtrlBlockshmPtr = shmat(ProcessCtrlBlockshmID, NULL, 0);
    if(ProcessCtrlBlockshmPtr < 0){
        perror("PCB shmat error in oss\n");
        exit(errno);
    }
}

#endif