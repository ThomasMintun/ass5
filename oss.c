#include <stdio.h> //input stderr
#include <stdlib.h> //for exit
#include <sys/ipc.h> // shared memory
#include <sys/shm.h> // shared memory
#include <unistd.h> // execl
#include <signal.h> //signal handling
#include <sys/types.h> // for wait
#include <sys/wait.h> // for wait and kill
#include <time.h> // for setperiodic
#include <string.h> // for memset
#define BILLION 1000000000L //for setperiodic


struct mesg_buffer {
	long mesg_type;
	int pid;
	int release;
	int request;
	int terminate;
} message;

struct shm{
	int clock[2];
	int resources [20]; // amount of each resource that exists
    int allocated [18][20]; // where the resources are allocated
    int max[18]; // max amount a resource a process could request
	int request[18]; //aligns with pidArr holds value of requested resource
	int pidArr[18]; //array of running processes
	int available[20]; //how many resources are left
	int blockTime[2]; //total time programs were waiting
}sharedmem;

//Global Vars
FILE *action;
key_t shmkey; // key for shared memory
int shmid; // id of shared memory
key_t mskey; //message key
int msid; //message id
struct shm *shared; // shared memory struct
int line = 0; //track file size
int blocked[18]; // blocked queue
int grants;
int bankrun;
int requests;


//deadlock detection
int bankers(int processes[], int avail[], int maxm[], int allot[][20], int resources[]);

//find pid in pidArr
int findPid(int pid);

//terminates program
void terminator();	

//signal handler
void sigHandler(int signo);

//convert nanoseconds to seconds
void timeConvert(int clock[]);

//countdown taken from book
static int setperiodic(double sec);

//print state of allocated resources
void printState();

//calcualte need
void findNeed(int need[18][20], int maxm[18], int allot[18][20]);

//calculate available
void findAvailable(int resources [], int alloted[][20], int available[]);

int main(int argc, char *argv[]){
	
	int verb = 1;
	int opt;
        while((opt = getopt (argc, argv, "hv")) != -1)
                switch (opt){
                        case 'h':
                                printf("Program spawns user children, uses some shared memory, and message queues.\n");
                                printf("Program will write to and create  stderr log stderr.log \n");
                                printf("Using option v turns verbose off only printing blocks and wakeups\n");
                                return 0;
                                break;
			case 'v':
				verb = 0;
				break;}

	//variables
	time_t t; //for time seeding
	shmkey = ftok("oss", 53); // key for shared memory
    mskey = ftok("oss", 51); // generate key for crit sec
	int spawnTime[2] ={ 0, 0}; // time when to spawn more processes
	int i; // for incrementing
	grants = 0;
	bankrun = 0;
	requests = 0;
	int closed = 0;

	//signal handler
	if (signal(SIGINT, sigHandler) == SIG_ERR)
                perror("oss: can't catch SIGINT");
    if (signal(SIGALRM, sigHandler) == SIG_ERR)
                perror("master: can't catch SIGALRM");
	if (signal(SIGSEGV, sigHandler) == SIG_ERR)
                perror("master: can't catch SIGSEGV");

	//set timer
	if (setperiodic(2) == -1) {
                perror("Failed to setup termination timer");
                return 1;
    }

	//seed random generator
	srand((unsigned) time(&t) + getpid());

	//open file for stderr
	if( !(action = fopen("action.log", "a"))){
		perror("oss: Error: unable to open stderr.log");
	}

	//create shared memory
	if((shmid = shmget(shmkey, (sizeof(sharedmem)), IPC_CREAT | 0666)) == -1){
                perror("oss: Error: shmget failed");
                return 1;
    }

	//create message queue
	if ((msid = msgget(mskey, IPC_CREAT | 0666)) == -1){
                perror("oss: Error: msgget");
                return 1;
    }

	//attach shared memory
	shared =  shmat(shmid, NULL, 0);

	//set arrays and resources
	memset((*shared).pidArr, 0, 18*sizeof(int));
	memset((*shared).clock, 0, 2*sizeof(int));
	memset((*shared).request, 0, 18*sizeof(int));
	memset(blocked, 0, 18*sizeof(int));
	memset((*shared).blockTime, 0, 2*sizeof(int));
	for(i = 0; i <20; i++){
                (*shared).resources[i] = (rand() % 8) + 3;
    }

	//main loop 
	while(1){
		//shutdown stderr
		if(line >= 100000 && closed == 0){
			closed = 1;
			fclose(stderr);                               
        }

		//spawn new processes
		if(((*shared).clock[1] == spawnTime[1] && (*shared).clock[0] >= spawnTime[0]) || (*shared).clock[1] > spawnTime[1]){

			int temp;
			//check if there is room for proc
			int newProc = 0; 
			for(i = 0; i < sizeof((*shared).pidArr)/sizeof(int); i++){
				if((*shared).pidArr[i] == 0){
					(*shared).max[i] = (rand() % 3) +1;
					temp = fork();
                    newProc = 1;
                    break;
                }
            }
			
			//if process spawned
			if(newProc == 1){
				if(temp  == -1){
					perror("oss: Error: Failed to fork");
                    kill(getpid(), SIGINT);
                }
                if(temp == 0){
                	execl("./user", NULL);
                	perror("oss: Error: exec failed for user process child");
                    return 1;
                }
				(*shared).pidArr[i] = temp;
				if(verb == 1){
				fprintf(stderr, "OSS: Spawning new process %i at %i:%09i \n", temp, (*shared).clock[1], (*shared).clock[0]);
				fflush(stderr);
                line++;}

			}else{
				if(verb == 1){
				fprintf(stderr, "OSS: Process limit currently full unable to spawn more processes. \n");
				fflush(stderr);
				line++;
				}
			}

			//reset spawn timer
			spawnTime[0] = (rand() % 500000000) + 1000000 + (*shared).clock[0];
	                spawnTime[1] = (*shared).clock[1];
	                timeConvert(spawnTime);
			if(verb == 1){
	                fprintf(stderr, "OSS: Will attempt to spawn new process at %i:%09i \n", spawnTime[1], spawnTime[0]);
	                fflush(stderr);
	                line++;}
				
		}

		msgrcv(msid, &message, sizeof(message), 2, IPC_NOWAIT);
		
		if(message.release != 0 || message.terminate !=0 || message.request != 0){
			i = findPid(message.pid);
		}	

		//if there is a request
		if (message.request != 0){

			requests++;			

			if(verb == 1){
			fprintf(stderr, "OSS: Request for resource %i recieved from process %i \n", message.request-1, message.pid);
			fflush(stderr);
			line++;}	
	
			//incrememnt clock for bankers
			(*shared).clock[0] += 10000;
	                timeConvert((*shared).clock);

			(*shared).allocated[i][message.request-1] += 1;	
	
			//if safe state allocate resource and increment clock
			bankrun++;
			if( bankers((*shared).pidArr, (*shared).available, (*shared).max, (*shared).allocated, (*shared).resources)==1){
				if(verb == 1){
				fprintf(stderr, "OSS: Request granted, giving process %i 1 resource of type %i at %i:%09i. \n", message.pid, message.request-1, (*shared).clock[1], (*shared).clock[0]);
                                fflush(stderr);
                                line++;}

				grants++;

				(*shared).clock[0] += 10000;
	                        timeConvert((*shared).clock);

				message.mesg_type = (*shared).pidArr[i];
        			if(msgsnd(msid, &message, sizeof(message), 0) == -1){
                		 	perror("user: Error: msgsnd");
				}		

				if ((grants % 20) == 0 && verb == 1){
				 	printState();
       				}

			// if not safe state place in blocked queue
			}else{
				(*shared).allocated[i][message.request-1] -= 1;
				fprintf(stderr, "OSS: Request denied, placing process %i into block queue. \n", message.pid);
                        	fflush(stderr);
                        	line++;

				(*shared).request[i] = message.request;
				int j;
			        for(j = 0; j < 18; j++){
                			if (blocked[j] == 0){
                        			blocked[j] = message.pid;
                        			break;
					}
				}

			} 
			message.request = 0;
		}
		// if there was a release
		if (message.release != 0 || message.terminate !=0){
			//if termination
			if(message.terminate == 1){
				(*shared).pidArr[i] = 0;
				(*shared).max[i] = 0;
				if(verb == 1){
				fprintf(stderr, "OSS: process %i terminated and released its resources at %i:%09i.\n", message.pid, (*shared).clock[1], (*shared).clock[0]);
                fflush(stderr);
                line++;}
				message.terminate = 0;
			}

			if(message.release != 0){
				if(verb == 1){
				fprintf(stderr, "OSS: process %i released its resource of type: %i.\n", message.pid, message.release);
                fflush(stderr);
                line++;}
				(*shared).allocated[i][message.release-1] -= 1;
				message.release = 0;
				message.mesg_type = (*shared).pidArr[i];
                if(msgsnd(msid, &message, sizeof(message), 0) == -1){
                perror("user: Error: msgsnd");
                }
			}
			//increment clock
			(*shared).clock[0] += 10000;
            timeConvert((*shared).clock);
			
			//check blocked queue
			int j;
			for(j = 0; j <18; j++){
				if(blocked[j] == 0){
					continue;
				}
				i = findPid(blocked[j]);
				//increment
				(*shared).clock[0] += 10000;
                timeConvert((*shared).clock);
				(*shared).allocated[i][(*shared).request[i]-1] += 1;
				//if safe state
				bankrun++;
				if(bankers((*shared).pidArr, (*shared).available, (*shared).max, (*shared).allocated, (*shared).resources) == 1){
					fprintf(stderr, "OSS: Request granted, waking blocked process %i and giving 1 resource of type %i at %i:%09i. \n", blocked[j], (*shared).request[i]-1, (*shared).clock[1], (*shared).clock[0]);
                                	fflush(stderr);
                                	line++;
				//wake up blocked program
				message.mesg_type = blocked[j];
				if(msgsnd(msid, &message, sizeof(message), 0) == -1){
				perror("user: Error: msgsnd");
                        		}
					//dequeue
					blocked[j] = 0;
					int requeue = 0;
					for(i = 0; i< 17; i++){
						if (blocked[i] == 0 && blocked[i+1] != 0){
							requeue = 1;
						}
						if (requeue ==1){
							blocked[i] = blocked[i+1];	
						}
					}
					blocked[i] = 0;
					//set to check from begining
					j = 0;
				}else{
					(*shared).allocated[i][(*shared).request[i]-1] -= 1;
				}
			}
		}


		//increment clock
		(*shared).clock[0] += 1000000;
		timeConvert((*shared).clock);

		//reap children
		while(waitpid(-1, NULL, WNOHANG) > 0);

	}

	terminator();
	return 0;
}


void findAvailable(int resources [], int alloted[][20], int available[]){
	int i;
	int j;
	for(i = 0; i < 20; i++){
		available[i] = resources[i];
		for(j = 0; j < 18; j++){
			available[i] -= alloted[j][i];
		}
	}
}

void printState(){
	int i, j;
	fprintf(stderr, "     ");
	for(j = 0; j < 20; j++){
		 fprintf(stderr, "R%02i ", j);
    }
	fprintf(stderr, "\n");

	for(i = 0; i < 18; i++){
		fprintf(stderr, "P%02i:", i);
		 for(j = 0; j < 20; j++){
			fprintf(stderr, "%4i", (*shared).allocated[i][j]);
			line++;
	        }
		 fprintf(stderr, "\n");
	}

	fprintf(stderr, "Proc: ");
	for(i = 0; i < 18; i++){
		 fprintf(stderr, "%i ", (*shared).pidArr[i]);
	}

	fprintf(stderr, "\nblocked: ");
        for(i = 0; i < 18; i++){
                 fprintf(stderr, "%i ", blocked[i]);
        }
	fprintf(stderr, "\n");
}

void findNeed(int need[18][20], int maxm[18], int allot[18][20]){
	int i;
	int j;
	for (i = 0 ; i < 18 ; i++){
 	    for (j = 0 ; j < 20 ; j++){
 	    	need[i][j] = maxm[i] - allot[i][j];
		}
	}
}


int bankers(int processes[], int avail[], int maxm[], int allot[][20], int resources[20]){
	findAvailable(resources, allot, avail);
	int need[18][20];
	findNeed(need, maxm, allot);
	int finish[18] = {0};

	//copy resources
	int work[20];
	int i;
    	for (i = 0; i < 20 ; i++){
        	work[i] = avail[i];
	}

	int cnt = 0;
    	while (cnt < 18){

		int found = 0;
		int p;
        	for (p = 0; p < 18; p++){

			//if process is "finished"
			if (finish[p] == 0){
				int j;
                		for (j = 0; j < 20; j++)
                    			if (need[p][j] > work[j]){
                        			break;
					}
				if (j == 20){
					int k;
					for (k = 0 ; k < 20 ; k++){
                        			work[k] += allot[p][k];
					}
						
					cnt++;
					finish[p] = 1;
					found = 1;
				}
			}
		}

		if (found == 0){
            	return 0;
        	}
    	}

	return 1;
}

int findPid(int pid){
	int i; 
	for(i = 0; i <18; i++){
                if((*shared).pidArr[i] == pid){
                        return i;
                }
        }
	kill(getpid(),SIGINT);
}

void terminator(){
	fprintf(stderr, "oss %i SIGINT received \n", getpid());
	printState();

	if(requests > 0){
		fprintf(stderr, "Total times bankers algorithm was run: %i \n", bankrun);
		fprintf(stderr, "Total times resource was requested: %i \n", requests);
		fprintf(stderr, "Total times resources were granted on initial request: %i \n", grants);
		double average = (double)grants/ (double)requests;
		fprintf(stderr, "Percentage of initial requests granted: %f \n", average);
		fprintf(stderr, "Total sum of time of all process spent waiting on request: %i:%09i\n", (*shared).blockTime[1], (*shared).blockTime[0]);
		fprintf(stderr, "Total running time of system: %i:%09i\n", (*shared).clock[1], (*shared).clock[0]);
	}

	//kill children
	int i;
	for(i = 0; i < sizeof((*shared).pidArr)/sizeof(int); i++){
                if((*shared).pidArr[i] != 0){
                        kill((*shared).pidArr[i], SIGINT);
                }
        }

	//wait and clear
	while(waitpid(-1, NULL, 0) > 0);
	if (shmctl(shmid, IPC_RMID, NULL) < 0){
		perror("oss: shmctl");
	}
        msgctl(msid, IPC_RMID, NULL);
}


void sigHandler(int signo){
        if (signo == SIGINT){
                terminator();
                exit(0);
        }
	if (signo == SIGSEGV){ 
		 fprintf(stderr, "\nProcess Seg Fault. \n");
                terminator();
                exit(0);
        }
	if (signo == SIGALRM){
		fprintf(stderr, "\nProcess Time out. \n"); 
                terminator();
                exit(0);
        }

}


void timeConvert(int clock[]){
        clock[1] += clock[0]/1000000000;
        clock[0] = clock[0]%1000000000;
}


static int setperiodic(double sec) {
   time_t timerid;
   struct itimerspec value;
   if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1)
      return -1;
   value.it_interval.tv_sec = (long)sec;
   value.it_interval.tv_nsec = (sec - value.it_interval.tv_sec)*BILLION;
   if (value.it_interval.tv_nsec >= BILLION) {
      value.it_interval.tv_sec++;
      value.it_interval.tv_nsec -= BILLION;
   }
   value.it_value = value.it_interval;
   return timer_settime(timerid, 0, &value, NULL);
}







