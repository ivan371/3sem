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

int receive (struct text* data_array, int shmid, int semid, struct sembuf* sem);

int main()
{
		key_t key = 0;
		struct text* data_array = 0;
		int shmid = 0;
		int semid = 0;
		struct sembuf sem[3] = {};
		
		key = ftok ("sender.c", 0);
		shmid = shmget (key, 1024, 0666 | IPC_CREAT);
		data_array = shmat (shmid, NULL, 0);
		key = ftok ("receiver.c", 0);
		semid = semget (key, 4, 0666 | IPC_CREAT);
		
		sem[0].sem_num = 2;
		sem[0].sem_op = 0;
		sem[0].sem_flg = IPC_NOWAIT; //To nonblock if copy is working
		sem[1].sem_num = 2;
		sem[1].sem_op = 1;
		sem[1].sem_flg = SEM_UNDO;

		//Exeption of copies
		if (semop (semid, &sem[0], 2) < 0) 
		{	
			perror ("Receiver is already working");
			return -1;
		}
		
		sem[0].sem_num = 0;
		sem[0].sem_op = -1;
		sem[0].sem_flg = 0;
		sem[1].sem_num = 0;
		sem[1].sem_op = 1;
		sem[1].sem_flg = 0;
		semop (semid, &sem[0], 2); //waiting for sender
		
		sem[0].sem_num = 3;
		sem[0].sem_op = -1;
		sem[0].sem_flg = IPC_NOWAIT;
		semop (semid, &sem[0], 1);
		
		while (receive (data_array, shmid, semid, sem))
		{
		}
		
		semctl (semid, 0, IPC_RMID, 0);
		shmdt (data_array);
    
		return 0;
}

int receive (struct text* data_array, int shmid, int semid, struct sembuf* sem)
{
		sem[0].sem_num = 1;
		sem[0].sem_op = 1;
		sem[0].sem_flg = 0;
		if (semop (semid, &sem[0], 1) < 0) 
		{ 
			perror ("1st sem + 1");
			return 0;
		}
		sem[0].sem_num = 0;
		sem[0].sem_op = -1;
		sem[0].sem_flg = IPC_NOWAIT;
		
		sem[1].sem_num = 0;
		sem[1].sem_op = 1;
		sem[1].sem_flg = 0;
		
		sem[2].sem_num = 3;
		sem[2].sem_op = -1;
		sem[2].sem_flg = 0;
		if (semop (semid, &sem[0], 3) < 0 && errno != EINVAL) 
		{
			perror("sender is not working");
			return 0;
		}
		write (0, data_array -> data, data_array -> numb);
		if (!data_array -> numb) 
			return 0;
		return 1;
}
