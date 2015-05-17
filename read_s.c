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
	key_t key = ftok("test.txt", 'a');
	int shmid = shmget(key, BUF, IPC_CREAT | 0660);
	if(shmid == -1)
	{
		printf("ERROR_IN_CREATR_SHMID");
		exit(1);
	}
	key_t key_2 = ftok("test.txt", 'b');
	int sid = semget(key_2, 4, IPC_CREAT|0660);
	char* segm = shmat(shmid, 0, 0);
	char* str = (char*)calloc(BUF, sizeof(char));
	struct sembuf sem_break = {0, -1, IPC_NOWAIT};
	struct sembuf par = {1, 1, 0};
	struct sembuf good_par = {1, 1, 0};
	struct sembuf unpar = {1, -1, 0};
	struct sembuf child = {2, 1, 0};
	struct sembuf unchild = {2, -1, 0};
	struct sembuf alive = {3, 1, 0};
	struct sembuf died = {3, -1, IPC_NOWAIT};
	
	ALIVE
	while(read(file, str, BUF) > 0)
	{	
		DIED
		CHILD	
		strcpy(segm, str);
		str = (char*)calloc(BUF, sizeof(char));		
		UNPAR
		ALIVE	
	}
	strcpy(segm, str);
	printf("%s", str);
	BREAK
	semctl(sid, 0, IPC_RMID, 0);
	shmdt(segm);
	shmctl(shmid, IPC_RMID, 0);
}
