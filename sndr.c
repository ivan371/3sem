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

#define ASSERT_snd( cond, message ) \
 if (!(cond)) {                     \
    printf ("(sender) ");           \
    perror (message);               \
    putchar ('\n');                 \
    return -1;                      \
 }

#define smart_ASSERT_snd( cond, message ) \
 if (!(cond)) {                           \
    semctl (semid, IPC_RMID, 0);          \
    shmctl (shmid, IPC_RMID, 0);          \
    printf ("(sender) ");                 \
    perror (message);                     \
    putchar ('\n');                       \
    return -1;                            \
 }


const int BUF_SIZE = 10;
const int NSEMS = 9;
const int SOPS_SIZE = 9;

const int SEM_SND_CAN_START = 0;
const int SEM_RCV_CAN_START = 1;
const int SEM_RCV_READY     = 2;
const int SEM_SND_READY     = 3;
const int SEM_SND_ALIVE     = 4;
const int SEM_RCV_ALIVE     = 5;
const int SEM_SND_MUTEX     = 6;
const int SEM_RCV_MUTEX     = 7;
const int SEM_SUCCESS       = 8;

int set_sop (struct sembuf *sops, int sop_num,
             unsigned short sem_num, short sem_op, short sem_flg);

int main (int argc, char** argv)
{
//=============== CHECKING_MAIN_ARGS ========================================
    ASSERT_snd (argc == 2,
                "ERROR: the program needs one argument");

//=============== CREATING_SEMAPHORES_&_SOPS ================================
    ASSERT_snd (creat ("key_file", 0644) != -1,
                "ERROR: creat (sem, key_file) failed");

    int semkey = ftok ("key_file", 2);
    ASSERT_snd (semkey != -1,
                "ERROR: ftok (sem) failed");

    int semid = semget (semkey, NSEMS, IPC_CREAT | 0644);
	ASSERT_snd (semid != -1,
                "ERROR: semget failed");

    struct sembuf sops[SOPS_SIZE];

//=============== TRYING_TO_START_WORK ======================================
    set_sop (sops, 0, SEM_SND_CAN_START, 0, IPC_NOWAIT);
    set_sop (sops, 1, SEM_SND_CAN_START, 1, SEM_UNDO);//if terminates, other senders can start
    set_sop (sops, 2, SEM_RCV_MUTEX,     1, SEM_UNDO);//!!!!!!!
    set_sop (sops, 3, SEM_SND_READY,     1, SEM_UNDO);//if terminates, no senders will be ready

    if (semop (semid, sops, 4) == -1) return 0;

//=============== CREATING_SHARED_MEMORY_&_ATTACHING ========================
    ASSERT_snd (creat ("key_file", 0644) != -1,
                "ERROR: creat (shm, key_file) failed");

    int shmkey = ftok ("key_file", 1);
    ASSERT_snd (shmkey != -1,
                "ERROR: ftok (shm) failed");

    int shmid = shmget (shmkey, BUF_SIZE + sizeof(int) + 1, IPC_CREAT | 0644);
	ASSERT_snd (shmid != -1,
                "ERROR: shmget failed");

    void* shmptr = shmat (shmid, NULL, 0);

//=============== SOME_PREPARATIONS =========================================
    int input_d = open (argv[1], O_RDONLY);
    smart_ASSERT_snd (input_d > 2,
                "ERROR: open (input) failed");

    int nBytes = BUF_SIZE;

//=============== SENDING_THE_FILE_TO_RECIEVER ==============================
    set_sop (sops, 0, SEM_RCV_READY,    -1, 0);
    set_sop (sops, 1, SEM_SND_CAN_START, 1, 0);
    set_sop (sops, 2, SEM_SND_ALIVE,     1, 0);
    set_sop (sops, 3, SEM_SND_ALIVE,    -1, SEM_UNDO);
    semop   (semid, sops, 4);

    while (1) {
        set_sop (sops, 0, SEM_RCV_ALIVE, 0, IPC_NOWAIT);// checking, that reciever is alive
        smart_ASSERT_snd (semop (semid, sops, 1) != -1,
                    "ERROR: Reciever is dead");
        set_sop (sops, 0, SEM_SND_MUTEX, 0, 0);// waiting for news from reciever, if sender can continue
        smart_ASSERT_snd (semop (semid, sops, 1) != -1,
                    "ERROR: something went wrong while waiting for snd_mutex == 0");
        nBytes = read (input_d, shmptr, BUF_SIZE);
        if (nBytes != BUF_SIZE) break;

        set_sop (sops, 0, SEM_RCV_MUTEX, -1, IPC_NOWAIT);
        set_sop (sops, 1, SEM_SND_MUTEX,  1, 0);
        smart_ASSERT_snd (semop (semid, sops, 2) != -1,
                          "ERROR: failed P(rcv_mutex)");
    }

//=============== FINISHING_WORK ============================================
    memcpy (&(shmptr[BUF_SIZE]), (void*) &nBytes, sizeof (int));
    set_sop (sops, 0, SEM_SND_MUTEX,  1, 0);
    set_sop (sops, 1, SEM_SUCCESS,    1, 0);
    set_sop (sops, 2, SEM_RCV_MUTEX, -1, 0);
    semop (semid, sops, 3);

    shmdt  (shmptr);
    close  (input_d);

    return 0;
}

int set_sop (struct sembuf *sops, int sop_num,
             unsigned short sem_num, short sem_op, short sem_flg)
{
    ASSERT_snd (sop_num < SOPS_SIZE,
                "ERROR: sop_num is too big");

    sops[sop_num].sem_num = sem_num;
    sops[sop_num].sem_op  = sem_op;
    sops[sop_num].sem_flg = sem_flg;

    return 0;
}
