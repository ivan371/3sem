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

#define BUF 10
#define ERROR(x) do{printf(x); exit(1);} while(0);
enum SENDING_SEMS
{
	SND_MUTEX,
	RCV_MUTEX,
	SND_DIED,
	RCV_DIED,
	RCV_CONNECT,
};
int snd_exit(int sid, int file, int shmid);
void* get_memptr(const char* filename, size_t size, int* id_to_save);
int main(int argc, char* argv[])
{
	if(argc != 2)
		ERROR("INVALID INPUT");
	int file = open(argv[1], 'r');
	int shmid = 0;
	void* smbuf = get_memptr("read.c", BUF + sizeof(long), &shmid);
	long* nbytes_to_save = smbuf + BUF;
	key_t key = ftok("read.c", 'b');
	int sid = semget(key, 5, IPC_CREAT|0660);
	struct sembuf snd_ifdie[2] = {	
		{SND_DIED, 1, 0},
		{SND_DIED, -1, SEM_UNDO}
	};
	if (semop(sid, &snd_ifdie[0], 1) == -1);
	//	ERROR("ERROR IN SND_IFDEE[0]");
	
	if (semop(sid, &snd_ifdie[1], 1) == -1);
	//	ERROR("ERROR IN SND_IFDEE[1]");
	struct sembuf unlock_mutex_ifdead[2] = {
		{RCV_MUTEX, 1, SEM_UNDO},
		{RCV_MUTEX, -1, 0}
	};
	if(semop(sid, unlock_mutex_ifdead, 2) == -1)
		ERROR("ERROR IN RCV_MUTEX");

	struct sembuf rcv_activate[3] = {
		{RCV_MUTEX, 1, 0},
		{RCV_CONNECT, 1, 0},
		{RCV_CONNECT, 0, 0}
	};
	semop(sid, rcv_activate, 2);
	//if(semop(sid, &rcv_activate[2], 1) == -1)
	//	ERROR("ERROR TO CONECT WITH RECIEVER");
	struct sembuf mutexes[4] = {
		{SND_MUTEX, 1, 0},
		{RCV_MUTEX, -1, 0},
		{RCV_DIED, 0, IPC_NOWAIT},
		{SND_MUTEX, 0, 0}
	};
	char* str = (char*)calloc(BUF, sizeof(char));
	int nbytes = 0;
	while ((nbytes = read(file, smbuf, BUF)) > 0)
	{
		 *nbytes_to_save = nbytes;
		semop(sid, &mutexes[0], 2);
		if (semop(sid, &mutexes[2], 2) == -1)
		{
			printf("Receiver dead\n");
			snd_exit(sid, file, shmid);
		}
	}
	semop(sid, &mutexes[0], 2);
	if (semop(sid, &mutexes[2], 2) == -1)
	{
		printf("Receiver dead\n");
		snd_exit(sid, file, shmid);

	}
	snd_exit(sid, file, shmid);
}
int snd_exit(int sid, int file, int shmid)
{
	if (sid >= 0)
		semctl(sid, 0, IPC_RMID);
	if (file >= 0)
		close(file);
	if (shmid >= 0)
		shmctl(shmid, IPC_RMID, NULL);
	exit(0);
}

void* get_memptr(const char* filename, size_t size, int* id_to_save)
{
	if (creat(filename, 0660) == -1)
		ERROR("CREATE FILE");
	key_t file_key = ftok(filename, 1);
	*id_to_save = shmget(file_key, size, IPC_CREAT |IPC_EXCL| 0660);	
	void* shm_ptr = shmat(*id_to_save, NULL, 0);
	memset(shm_ptr, size, 0);
	return shm_ptr;
}
