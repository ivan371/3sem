#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>
#include <sys/sem.h>
#include <fcntl.h>

#define PAR if((semop(sid, &par, 1) == -1)) perror("semop");

#define UNPAR if((semop(sid, &unpar, 1) == -1)) perror("semop");

#define CHILD if((semop(sid, &child, 1) == -1)) perror("semop");

#define UNCHILD if((semop(sid, &unchild, 1) == -1)) perror("semop");

#define ALIVE if((semop(sid, &alive, 1) == -1)) printf("ALIVE_ERROR"); 

#define DIED if((semop(sid, &died, 1) == -1))\
{\
	printf ("ERROR! WRITE_IS_DIED");\
	semctl(sid, 0, IPC_RMID, 0);\
	shmdt(segm);\
	shmctl(shmid, IPC_RMID, 0);\
	exit(1);\
}
#define BUF 10

#define BREAK semop(sid, &sem_break, 1); ALIVE 
void sh_mem(char* filename);
void first_change(int sid);
void ch_sem(int sid, int shmid, char* segm); 

int main(int argc, char* argv[])
{
	int num = 0;
	char* end = 0;
	if(argc != 2)
	{
		printf("ERROR");
	}
	else
	{	
		sh_mem(argv[1]);
	}
	return 0;
}

void sh_mem(char* filename)
{
	unsigned int file = open(filename, 'r');
	key_t key = ftok("read.c", 'a');
	int shmid = shmget(key, BUF, IPC_CREAT | 0660);
	if(shmid == -1)
	{
		printf("ERROR_IN_CREATR_SHMID");
		exit(1);
	}
	key_t key_2 = ftok("read.c", 'b');
	int sid = semget(key_2, 4, IPC_CREAT|0660);
	void* segm = (void*)shmat(shmid, NULL, 0);
	memset (segm, BUF + sizeof(long), 0);
	long* bytes_to_save = segm + BUF;
	char* str = (char*)calloc(BUF, sizeof(char));
	first_change(sid);
	int nbytes = 0;
	while(nbytes = read(file, &segm, BUF) > 0)
	{	
		//*bytes_to_save = nbytes;
		ch_sem(sid, shmid, segm);
		//strcpy(segm, str);
		//printf("%s", segm);
		//str = (char*)calloc(BUF, sizeof(char));		
	}
	//strcpy(segm, str);
	//printf("%s", str);
	semctl(sid, 0, IPC_RMID, 0);
	shmdt(segm);
	shmctl(shmid, IPC_RMID, 0);
}


void first_change(int sid)
{
	struct sembuf sem[4] = 	{
					{0, 4, IPC_NOWAIT},
					{1, 5, IPC_NOWAIT},
					{2, 1, SEM_UNDO},
					{3, 1, SEM_UNDO}
				};
	semop(sid, sem, 4);
	struct sembuf newsem[2] = 	{
					{0, -4, SEM_UNDO},
					{1, -4, SEM_UNDO}	
				};
	semop(sid, newsem, 2);
}

void ch_sem(int sid, int shmid, char* segm)
{
	struct sembuf sem[3] = 	{
					{0, 1, 0},
					{1, -1, 0},
					{2, -1, IPC_NOWAIT}
				
				};
	if(semop(sid, sem, 3) == -1)
	{
		semctl(sid, 0, IPC_RMID, 0);
		shmdt(segm);
		shmctl(shmid, IPC_RMID, 0);
		exit(1);
	}
	struct sembuf newsem = {2, 1, IPC_NOWAIT};
	semop(sid, &newsem, 1);
}
