#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <string.h>

#define ERROR(x) \
    do { printf(x "\n");  exit(-1);} while(NULL);

#define BUF 10

enum sems
{
	SEM_SND_CAN_START,
	SEM_RCV_CAN_START,
	SEM_RCV_READY,
	SEM_SND_READY,
	SEM_SND_ALIVE,
	SEM_RCV_ALIVE,
	SEM_SND_MUTEX,
	SEM_RCV_MUTEX,
	SEM_SUCCESS    
};

void start_sender(int sid);
int main (int argc, char** argv)
{
	if(argc != 2)
		ERROR("INVALID INPUT")
	int file = open(argv[1], O_RDONLY);
	key_t key = ftok("read.c", 'b');
	int sid = semget(key, 9, IPC_CREAT|0660);
	start_sender(sid);

	int shmid = shmget(key, BUF, IPC_CREAT | 0660);
	if(shmid == -1)
		ERROR("ERROR IN SHMID");
	char* segm = shmat(shmid, NULL, 0);
	void* tmp_segm = (void*)calloc(BUF, sizeof(void));
	char* str = (char*)calloc(BUF, sizeof(void));
	int nBytes = BUF;
	while(1)
	{
		struct sembuf sem1[3] = {
					{SEM_RCV_ALIVE, -1, IPC_NOWAIT},
					{SEM_RCV_ALIVE, 1, 0},
					{SEM_SND_MUTEX, -1, 0}
					};
		if(semop(sid, sem1, 3) == -1)
		{
			semctl (sid, IPC_RMID, 0);          
    			shmctl (shmid, IPC_RMID, 0); 
			ERROR("RECIEVER DIED");
		}
		nBytes = read (file, segm + 4, BUF);
		*(int*)segm = nBytes; 		
		struct sembuf sem[2] = {
				{SEM_RCV_MUTEX, 1, 0},
				{SEM_SND_MUTEX,  1, 0}
				};
		semop(sid, sem, 1);
		if(nBytes != BUF)
		{
			break;
		}
	}
	shmdt  (segm);
    	close  (file);
}

void start_sender(int sid)
{
	struct sembuf sem[4] = {
				{SEM_SND_CAN_START, 0, IPC_NOWAIT},
				{SEM_SND_CAN_START, 1, SEM_UNDO},
				{SEM_SND_ALIVE,     1, SEM_UNDO},
				{SEM_SND_READY,     1, SEM_UNDO}
				};
	if(semop(sid, sem, 4) == -1)
		ERROR("START SEM ERROR");

	struct sembuf sem2 = {SEM_SND_MUTEX, -1, IPC_NOWAIT};
	semop(sid, &sem2, 1);
	struct sembuf sem1[2] = {
				{SEM_RCV_READY,     -1, 0},
				{SEM_RCV_READY,    1, 0}
					};
	semop(sid, sem1, 2);
}
