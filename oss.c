#include "oss.h"

void ossClock();
void sigint(int);
void forkChild();
void initForkToPCB(pid_t holder);
void printLog();
void printStats();
static void ALARMHandler();

int second = 0, c, MaxUserProcesses = 18, totalLines = 1000;
int queue[18] = {0};

int main(int argc, char* argv[]){
    //2 second alarm
    signal(SIGALRM, ALARMHandler);
    alarm(2);
    int clockStop = 0;//flag for stopping OS clock
    time_t t;//for init random
    int forkMax = 0;
    srand((unsigned) time(&t));//init clock the random
    signal(SIGINT, sigint);//signal handling config for ctrl-c
    sharedMemoryConfig();//configure shared memory

    //initialize queues to 0 for priority queues

    //message passing varibales

    //attach message queues

    //clock
    while(clockStop == 0){
        ossClock();//increments clock and adjusts sec:nanos

        forkChild();//spawn child
        //then add child to the queue
        //

        printStats();
        printLog();
    }

    // clean shared memory
    shmdt(sysClockshmPtr);
    return 0;
}
void printStats() {
    FILE *fp = fopen("stats.txt", "a+");
    fprintf(fp, "\t\tAverage Turnaround Time: %f seconds\n");
    fprintf(fp, "\t\tAverage Wait Time in Queue: %f seconds\n");
    fprintf(fp, "\t\tAverage CPU Usage by Processes: %f seconds\n");
    fprintf(fp, "\t\tTotal CPU Down Time: %f seconds\n");
}

static void ALARMHandler(){
    shmdt(sysClockshmPtr);
    shmctl(sysClockshmid, IPC_RMID, NULL);
    shmctl(ProcessCtrlBlockshmPtr, IPC_RMID, NULL);
    shmctl(ProcessCtrlBlockshmID, IPC_RMID, NULL);
    printf("2 seconds are up!\n");
    exit(EXIT_SUCCESS);
}

//signal handles ctrl-c
void sigint(int a){
    printf("Control+C caught. Will now delete shared memory.\n");
    //clean shared memory
    shmdt(sysClockshmPtr);
    shmctl(sysClockshmid, IPC_RMID, NULL);
    shmdt(ProcessCtrlBlockshmPtr);
    shmctl(ProcessCtrlBlockshmID, IPC_RMID, NULL);
    exit(0);
}

//increments the clock random value and adjust sec:nanos
void ossClock(){
    int clockIncrement;//random number to increment time
    int extraTime;//extraTime nanos

    clockIncrement = (unsigned int) (rand() % 600000000 + 1);//move this to 10,000 when 

    if ((sysClockshmPtr->nanoseconds + clockIncrement) > 999999999){
        extraTime = (sysClockshmPtr->nanoseconds + clockIncrement) - 999999999;
        sysClockshmPtr->seconds += 1;
        sysClockshmPtr->nanoseconds = extraTime;
        second = 1;
    } else {
        sysClockshmPtr->nanoseconds += clockIncrement;
    }

    //printf("%d\n", sysClockshmPtr->seconds);
    //printf("%d\n", sysClockshmPtr->nanoseconds);

}

void forkChild(){ //fork a child every one second
    if(second == 1){//everytime second flips it forks a new child
        for( c = 0; c < sizeof(queue)/sizeof(int); c++){
            if(queue[c] == 0){
                if ((queue[c] = fork()) == 0){
                    execl("./user", "user", NULL);
                }
            ProcessCtrlBlockshmPtr[c]->pidHolder = queue[c];

            initForkToPCB(queue[c]);

            ProcessCtrlBlockshmPtr[c]->complete = 0;
            break;
            }
        }second--;
    }
}

void initForkToPCB(pid_t holder){
    ProcessCtrlBlockshmPtr[0]->startCPUNanoS = rand() % 2;
    ProcessCtrlBlockshmPtr[0]->startCPUSeconds = (rand() % 99) * 10000000;

    FILE *fp = fopen("loggit.txt", "a+");
    fprintf(fp, "Generating process with ProcessID %d (priority),"
                " and putting it in queue (number) at time "
                "%d : %d \n", holder, sysClockshmPtr[0].seconds, sysClockshmPtr[0].nanoseconds);
    fclose(fp);
}

void printLog(){
    FILE *fp = fopen("log.txt", "a+");

    int ii, jj;

    // init max table
    fprintf(fp, "##### MAX TABLE #####\n");
    fprintf(fp, "     ");
    for(jj = 0; jj < 20; jj++){
        fprintf(fp, "R%02i ", jj);
    }

    fprintf(fp, "\n");

    for(ii = 0; ii < 18; ii++){
        fprintf(fp, "P%02i:", ii);
        for(jj = 0; jj < 20; jj++){
            fprintf(fp, "%4d", RDPtr->max[ii][jj]);
            totalLines++;
        }
        fprintf(fp, "\n");
    }

    // for initial rescources
    fprintf(fp, "\n##### RESCOURCES #####\n");
    fprintf(fp, "     ");
    for(jj = 0; jj < 20; jj++){
        fprintf(fp, "R%02i ", jj);
    }

    fprintf(fp, "\n    ");

    for(jj = 0; jj < 20; jj++){
        fprintf(fp, "%4d", RDPtr->rescources[jj]);
    }

    fprintf(fp, "\n");

    // init max table
    fprintf(fp, "##### REQUEST TABLE #####\n");
    fprintf(fp, "     ");
    for(jj = 0; jj < 20; jj++){
        fprintf(fp, "R%02i ", jj);
    }

    fprintf(fp, "\n");

    for(ii = 0; ii < 18; ii++){
        fprintf(fp, "P%02i:", ii);
        for(jj = 0; jj < 20; jj++){
            fprintf(fp, "%4d", RDPtr->request[ii][jj]);
            totalLines++;
        }
        fprintf(fp, "\n");
    }

    fprintf(fp, "\n");

    fprintf(fp, "##### JOBS #####\n");
    for(ii = 0; ii < 18; ii++){
        fprintf(fp, "%d    ", RDPtr->pidJob[ii]);
    }

    fprintf(fp, "\n");

    fprintf(fp, "##### TIME INTERVALS #####\n");
    for(ii = 0; ii < 18; ii++){
        fprintf(fp, "%d    ", (rand() % 500000000) + 1000000);
    }

    fprintf(fp, "\n");

    fclose(fp);
}

void addToQueue(int queue, pid_t pid){
    FILE *fp = fopen("log.txt", "a+");
    switch (queue){
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
            printf("Error: Could not add child #%i to a queue!\n", pid);
            fprintf(stderr, "Error adding to queue\n");
            fprintf(fp, "Error: Could not add child #%i to a queue!\n", pid);
            break;
    }
}