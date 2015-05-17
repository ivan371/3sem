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

enum RECEIVE_SEMS
{
	SND_MUTEX,
	RCV_MUTEX,
	SND_DIED,
	RCV_DIED,
	SND_CONNECT,
};
void* get_memptr(const char* filename, size_t size, int* id_to_save);
int main()
{
	int shmid = 0;
	void* smbuf = get_memptr("read.c", BUF + sizeof(long), &shmid);
	long* nbytes_to_save = smbuf + BUF;
	key_t key = ftok("read.c", 'b');
	int sid = semget(key, 4, IPC_CREAT|0660);
	struct sembuf sem_undo_zero[2] = {
		{RCV_DIED, 1, 0},
		{RCV_DIED, -1, SEM_UNDO}};
	semop(sid, &sem_undo_zero[0], 1);
	//	ERROR("ERROR IN RCV_DIED[0]");
	semop(sid, &sem_undo_zero[1], 1);
	//	ERROR("ERROR IN RCV_DIED[1]");
	struct sembuf unlock_mutex_ifdead[2] = {
		{SND_MUTEX, 1, SEM_UNDO},
		{SND_MUTEX, -1, 0}
	};
	if(semop(sid, unlock_mutex_ifdead, 2) == -1);
	//	ERROR("ERROR IN SND_MUTEX");

	//char* segm = shmat(shmid, 0, 0);
	struct sembuf mutexes[5] = {
		{SND_MUTEX, -1, 0},
		{RCV_MUTEX, 1, 0},
		{SND_DIED, 0, IPC_NOWAIT},
		{RCV_MUTEX, 0, 0},
		{SND_CONNECT,-1, 0},
	};
	//memset(shm_ptr, BUF, 0);
	//long* nbytes_to_save = 1;
	//if(semop(sid, &mutexes[4], 1) == -1);
	//	ERROR("ERROR TO CONNECT WITH SENDER");
	semop(sid, &mutexes[2], 2);
	char str[10] = {};
	while (*nbytes_to_save != -1)
	{
		if (write(STDOUT_FILENO, str, *nbytes_to_save) != *nbytes_to_save)
			fprintf(stderr, "%ld\n", *nbytes_to_save);
		semop(sid, &mutexes[0], 2);
		if (semop(sid, &mutexes[2], 2) == -1)
		{
			printf("Sender dead\n");
			exit(1);
		}
	};
	semop(sid, &mutexes[0], 1);
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
