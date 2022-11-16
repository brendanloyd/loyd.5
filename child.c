#include "child.h"

int main(int argc, char** argv) {
	//variables for message queue
	message buf;
        key_t key;
        int msqid = 0;
        const int key_id = 1234;

	//setup id for the second clock
	int segment_id = shmget(SHMKEY, BUFF_SZ, 0777);
        if (segment_id == -1) {
                perror("Error: child.c : shared memory failed.");
        }

	//setup id for the nano second clock
        int nano_segment_id = shmget ( NANOKEY, BUFF_SZ, 0777 | IPC_CREAT);
        if (nano_segment_id == -1) {
                perror("Error: parent.c : shared memory failed for nano clock");
        }
	
	//attach id for second clock
	int* second_clock = (int*)(shmat(segment_id, 0, 0));
	if (second_clock == NULL) {
                perror("Error: child.c : shared memory attach failed.");
        }
	
	int resourceArray_segment_id = shmget ( ARRAYKEY, sizeof(int) * 20, 0777 | IPC_CREAT);
        if (resourceArray_segment_id == -1) {
                perror("Error: parent.c : shared memory failed.");
        }

	//attach id for nano second clock;
        int* nano_clock = (int*)(shmat(nano_segment_id, 0, 0));
        if(nano_clock == NULL) {
                perror("Error: parent.c : shared memory for nano clock failed.");
        }

        int* resourceArray = (int*)(shmat(resourceArray_segment_id, 0, 0));
        if(resourceArray == NULL) {
                perror("Error: parent.c : shared memory for array failed.");
	}

	//setup message queue	
	key = ftok("./parent.c", key_id);
        msqid = msgget(key, 0644|IPC_CREAT);
	buf.mtype = 1;
	buf.resource = 2;

	//random values needed for system times
	srand(time(NULL));                
	//start 3 seconds and spawn children
	time_t endwait = time(NULL) + 2;
	while(time(NULL) < endwait) {
		if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1) {
                        perror("msgsnd");
                        exit(1);
                }
	
		// recieve message if there is one, if not wait.
		msgrcv(msqid, &buf, sizeof(buf), 1, 0);
		printf("resource granted\n");

	}
	buf.resource = -1;
	if (msgsnd(msqid, &buf, sizeof(buf), 0) == -1) {
                perror("msgsnd");
        	exit(1);
        }	
	//detach shared memory and return success
	shmdt(second_clock);
	shmdt(nano_clock);
	return EXIT_SUCCESS;
}


