#include "parent.h"

//signal handler function to kill program if ctrl c is encountered
void terminateSigHandler(int sig) {
		printf("SIGINT signal encountered. Terminating.\n");
		printf("Clock value is: %d\n",*mem_ptr);
                shmdt(mem_ptr);
                kill(0, SIGQUIT);
}

//signal handler function to timeout after a certain determined about of time
void timeoutSigHandler(int sig) {
	if(sig == SIGALRM) {
		printf("SIGALRM signal ecountered indicating a timeout. Terminating\n");
	 	printf("Clock value is: %d\n",*mem_ptr);
		shmdt(mem_ptr);	
		kill(0, SIGQUIT);	
	}

}

void incrementClock(int nanoIncrement, int *second_clock, int *nano_clock) {
	*nano_clock += nanoIncrement;
	
	if(*nano_clock > 1000000000) {
		*nano_clock -= 1000000000;
		*second_clock += 1;
	}
}

void forkChildren(int num) {
        int i;
	for(i = 0; i < num; i++) {
                if (fork() == 0) {
                        char* args[] = {"./child", 0};
                        execlp(args[0],args[0],args[1]);
                        fprintf(stderr,"Exec failed, terminating\n");
                        exit(1);
                }
        }
}

int main(int argc, char **argv) {

	//Variables for signal handling
        signal(SIGTERM, terminateSigHandler);
        signal(SIGALRM, timeoutSigHandler);
        alarm(5);
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = terminateSigHandler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	//Variables for message queue
	message buf;
	key_t key;
	int msqid = 0;
	const int key_id = 1234;

	//Setup key for message queue
	key = ftok("./parent.c",key_id);

	//Setup id for message queue
        msqid = msgget(key, 0644|IPC_CREAT);

	//store primer message	
	buf.mtype = 1;
	buf.resource = 0;

	//get up seg id for second memory
	int segment_id = shmget ( SHMKEY, BUFF_SZ, 0777 | IPC_CREAT);
	if (segment_id == -1) {
		perror("Error: parent.c : shared memory failed.");
	}
	//set up seg id for array
	int resourceArray_segment_id = shmget ( ARRAYKEY, sizeof(int) * 20, 0777 | IPC_CREAT);
        if (segment_id == -1) {
                perror("Error: parent.c : shared memory failed.");
        }

	//set up seg id for nano second
	int nano_segment_id = shmget ( NANOKEY, BUFF_SZ, 0777 | IPC_CREAT);
	if (nano_segment_id == -1) {
		perror("Error: parent.c : shared memory failed for nano clock");
	}

	//attach memory segment for second clock
	int* second_clock = (int*)(shmat(segment_id, 0, 0));
	if (second_clock == NULL) {
		perror("Error: parent.c : shared memory attach failed.");
	} 

	//attach memory segment for nano second clock
	int* nano_clock = (int*)(shmat(nano_segment_id, 0, 0));
	if(nano_clock == NULL) {
		perror("Error: parent.c : shared memory for nano clock failed.");
	}
	
	//attach memory segment for array
	int* resourceArray = (int*)(shmat(resourceArray_segment_id, 0, 0));
	if(resourceArray == NULL) {
		perror("Error: parent.c : shared memory for array failed.");

	}

	FILE *out_file = fopen("resource.log", "w");

	int i = 0;
	for(i = 0; i < 20; i++) {
		resourceArray[i] = (rand() % 10) + 1;
	}
	
	/* Set shared memory segment to 0  */
	*second_clock = 0;
	*nano_clock = 0;

	//this is for signal handling
	mem_ptr = second_clock;

	//For wait command
	pid_t wpid;
	int status = 0;	

	//random values needed for system times
	srand(time(NULL));
	//start 3 seconds and spawn children
	time_t endwait = time(NULL) + 2;

	//setup a random time to fork child processes
	int timeToForkChild = (rand() % 500000);
	
	//variable to hold totalChildProcesses
	int totalChildProcesses = 0;
        while(time(NULL) < endwait) {
		incrementClock(10000, second_clock, nano_clock);
		if(*nano_clock < 15000) {
			forkChildren(1);
		}
		/*if(*nano_clock > timeToForkChild && totalChildProcesses < 40) {
			//forkChildren(1);
			totalChildProcesses++;
			printf("Simulating a child fork\n");
			timeToForkChild += (rand() % 500000);
		}*/
		
		msgrcv(msqid, &buf, sizeof(buf), 1, IPC_NOWAIT);
		printf("resource requested is: %d\n",buf.resource);

		if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1) {
                	perror("msgsnd : parent.c ");
                	exit(1);
        	}
 
        }

	//wait for all child processes to finish
	while ((wpid = wait(&status)) > 0);
	
	//print results
	fprintf(out_file,"Clock value in seconds is: %d : NanoSeconds is : %d\nParent is now ending\n",*second_clock, *nano_clock);
        printf("Clock value in seconds is: %d : NanoSeconds is : %d\nParent is now ending\n",*second_clock, *nano_clock);

	fprintf(out_file,"---Available resource array---\n");	
	for(i = 0; i < 20; i++) {
	fprintf(out_file, "%d  ",i);
	}
	fprintf(out_file, "\n");
	for(i = 0; i < 20; i++) {
		fprintf(out_file, "%d  ", resourceArray[i]);
		if(i > 10) {
			fprintf(out_file, " ");
		}
	}
	fprintf(out_file, "\n");
	for(i = 0; i < 20; i++) {
	printf("The value of the array at position :%d is: %d\n", i, resourceArray[i]);
	}
	//detach message queue memory	
	if (msgctl(msqid, IPC_RMID, NULL) == -1) {
      		perror("msgctl");
      		exit(1);
   	}
	
	//detach from the shared memory segment
	shmdt(second_clock);
	shmdt(nano_clock);
	shmdt(resourceArray);
	//free shared memory segment shm_id
	shmctl(segment_id, IPC_RMID, NULL);
	shmctl(nano_segment_id, IPC_RMID, NULL);
	shmctl(resourceArray_segment_id, IPC_RMID, NULL);
	return EXIT_SUCCESS; 
}

