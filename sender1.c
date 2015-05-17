#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

struct text 
{
    char data[1024 - sizeof (int)];
    int numb;
};

int send (struct text* data_array, int shmid, int semid, struct sembuf* sem, int file);

int main(int argc,char *argv[]) 
{
		key_t key = 0;
		struct text* data_array = 0;
		int shmid = 0;
		int semid = 0;
		int file = 0;
		struct sembuf sem[3] = {};
		
		if (argc != 2) 
		{
			printf ("usage: s1 filename");
			return -1;
		}
		
		key = ftok ("sender.c", 0);
		shmid = shmget (key, 1024, 0666 | IPC_CREAT);
		data_array = shmat (shmid, NULL, 0);
		key = ftok ("receiver.c", 0);
		semid = semget (key, 4, 0666 | IPC_CREAT);
		file = open (argv[1], O_RDONLY);
		
   
		sem[0].sem_num = 0;
		sem[0].sem_op = 0;
		sem[0].sem_flg = IPC_NOWAIT;	//block while (0th sem == 1) 
		sem[1].sem_num = 0;	// 0th sem - exeption of copies
		sem[1].sem_op = 1;
		sem[1].sem_flg = SEM_UNDO;  // 0th sem = 1
		if (semop (semid, &sem[0], 2) < 0) 
		{
			perror ("Sender is already working");
			return -1;
		}
		
		sem[0].sem_num = 2;
		sem[0].sem_op = -1;
		sem[0].sem_flg = 0;
		sem[1].sem_num = 2;
		sem[1].sem_op = 1;
		sem[1].sem_flg = 0;
		semop (semid, &sem[0], 2); //Waiting for receiver
		
		sem[0].sem_num = 1;
		sem[0].sem_op = -1;
		sem[0].sem_flg = IPC_NOWAIT;
		semop (semid, &sem[0], 1);
		
		while (send (data_array, shmid, semid, sem, file))
		{
		}

		semctl (semid, 0, IPC_RMID, 0);
		return 0;
}

int send (struct text* data_array, int shmid, int semid, struct sembuf* sem, int file)
{
		int read_flag = 0;
		
		sem[0].sem_num = 2;
		sem[0].sem_op = -1;
		sem[0].sem_flg = IPC_NOWAIT;
		
		sem[1].sem_num = 2;
		sem[1].sem_op = 1;
		sem[1].sem_flg = 0	; 
		
		sem[2].sem_num = 1;
		sem[2].sem_op = -1;
		sem[2].sem_flg = 0;
		
		if (semop (semid,&sem[0], 3) < 0) 
		{	
			perror("receiver is not working");
    	    return 0;
		}
		
		if ((read_flag = read (file, data_array -> data, 1024 - sizeof (int))) < 0) 
		{
			perror("1 reading");
			return 0;
		}
		data_array -> numb = read_flag;
		
		//Critical section
		sem[0].sem_num = 3;
		sem[0].sem_op = 1;
		sem[0].sem_flg = 0;
		if (semop (semid, &sem[0], 1) < 0) 
		{
			perror("3 sem+1");
			return 0;
		}
		if (!read_flag) 
			return 0;
		return 1;
}
