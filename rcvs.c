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

void start_reciever(int sid);
void finish(int sid);
int main (int argc, char** argv)
{
	if(argc != 1)
		ERROR("INVALID INPUT")
	key_t key = ftok("read.c", 'b');
	int sid = semget(key, 9, IPC_CREAT|0660);
	start_reciever(sid);

	int shmid = shmget(key, BUF, IPC_CREAT | 0660);
	if(shmid == -1)
		ERROR("ERROR IN SHMID");
	char* segm = shmat(shmid, NULL, 0);
	int file = open ("ttt.txt", O_WRONLY);
	int nBytes = BUF;
	while(1)
	{
		struct sembuf sem = {SEM_SND_MUTEX, 1, 0};
		semop(sid, &sem, 1);
		struct sembuf sem2[3] = {
				{SEM_SND_ALIVE, -1, IPC_NOWAIT},
				{SEM_SND_ALIVE, 1, 0},
				{SEM_RCV_MUTEX,  -1, 0}
				};
		if(semop(sid, sem2, 3) == -1)
		{	
			semctl (sid, IPC_RMID, 0);          
    			shmctl (shmid, IPC_RMID, 0);
			ERROR("ERROR IN SND MUTEX");
		}	
		nBytes = *(int*) segm;
		write(file, segm + 4, nBytes);
		write(STDOUT_FILENO, segm + 4, nBytes);		
		if (nBytes != BUF)
			break;	
	}
	shmdt  (segm);
	shmctl (shmid, IPC_RMID, 0);
	semctl (sid, IPC_RMID, 0);

}

void start_reciever(int sid)
{
	struct sembuf sem[4] = {
				{SEM_RCV_CAN_START, 0, IPC_NOWAIT},
				{SEM_RCV_CAN_START, 1, SEM_UNDO},
				{SEM_RCV_ALIVE,     1, SEM_UNDO},
				{SEM_RCV_READY,     1, SEM_UNDO}
				};
	if(semop(sid, sem, 4) == -1)
		ERROR("START RCV ERROR");
	struct sembuf sem2 = {SEM_RCV_MUTEX, -1, IPC_NOWAIT};
	semop(sid, &sem2, 1);

	struct sembuf sem1[2] = {
				{SEM_SND_READY,    -1, 0},
				{SEM_SND_READY,    1, 0},
				};
	semop(sid, sem1, 2);
}
