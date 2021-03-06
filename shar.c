#ifndef __SHARED_H
#define __SHARED_H
//===============================================================================================================
// Headers
//===============================================================================================================
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <fcntl.h>
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "assert.h"
#include "errno.h"
#include "limits.h"
#include <pthread.h>
#include <string.h>
//===============================================================================================================
// Macroses
//===============================================================================================================
//#define DEBUG
#define F_LOCATION(stream) \
fprintf(stream, "File: %s\n"	\
"Line: %d\n"	\
"Function: %s\n", \
__FILE__, \
__LINE__, \
__PRETTY_FUNCTION__); \
fflush(stream);
#define LOCATION F_LOCATION(stdout)
#ifdef DEBUG
#define OUT(str) printf(str); fflush(stdout);
#define OUT1(str, arg1) printf(str, arg1); fflush(stdout);
#define OUT2(str, arg1, arg2) printf(str, arg1, arg2); fflush(stdout);
#define OUT3(str, arg1, arg2, arg3) printf(str, arg1, arg2, arg3); fflush(stdout);
#define LOC_OUT(str) LOCATION; printf(str); fflush(stdout);
#define LOC_OUT1(str, arg1) LOCATION; printf(str, arg1); fflush(stdout);
#define LOC_OUT2(str, arg1, arg2) LOCATION; printf(str, arg1, arg2); fflush(stdout);
/*
CHECK(n != 1, "Error happened");
if (n != 1)
{
printf("Error happened");
perror("");
return -1;
}
*/
*/
#else
#define OUT(str) if (0) printf(str);
#define OUT1(str, arg1) if (0) printf(str, arg1);
#define OUT2(str, arg1, arg2) if (0) printf(str, arg1, arg2);
#define OUT3(str, arg1, arg2, arg3) if (0) printf(str, arg1, arg2, arg3);
#define LOC_OUT(str) if (0) printf(str);
#define LOC_OUT1(str, arg1) if (0) printf(str, arg1);
#define LOC_OUT2(str, arg1, arg2) if (0) printf(str, arg1, arg2);
#endif
#define F_CHECK_EXIT_CODE
#define F_CHECK(stream, cond, msg) \
if (!(cond)) \
{ \
F_LOCATION(stream); \
fprintf(stream, "Message: %s\n", msg); \
fflush(stream); \
perror("ERRNO\t\t"); \
fputc('\n', stream); \
F_CHECK_EXIT_CODE \
\
exit(EXIT_FAILURE); \
} \
#define CALL(func, var, cond, msg) \
var = func; \
CHECK(!((var)##cond), msg); \
#define CHECK(cond, msg) F_CHECK(stdout, cond, msg)
#define F_WARN(stream, cond, msg) \
if (!(cond)) \
{ \
fprintf(stream, "WARNING>>>>\n"); \
F_LOCATION(stream); \
fprintf(stream, "Message: %s\n", msg); \
fflush(stream); \
perror("ERRNO\t\t"); \
fputc('\n', stream); \
fprintf(stream, ">>>>>>>>>>>\n"); \
fflush(stream); \
} \
#define WARN(cond, msg) F_WARN(stdout, cond, msg)
//===============================================================================================================
// Constants (Code dependant)
//===============================================================================================================
const char* FILE_NAME_SHMEM_ATTACH = "SHARING.tmp";
const char* SND_FLAG = "SENDING.tmp";
const char* RCV_FLAG = "RECEIVING.tmp";
const long BUF_SIZE = 4 * 1024 * 1024;
const long BUF_SIZE_W_NBYTES = 4 * 1024 * 1024 + sizeof(long);
//#define BUF_SIZE (4 * 1024 * 1024)
//#define BUF_SIZE_WITH_NBYTES (4 * 1024 * 1024 + sizeof())
#define	PAGESIZE	getpagesize()
enum SHARING_SEMAPHORS
{
SND_FINISHED= 0,
};
enum SENDING_SEMS
{
SND_MUTEX,
RCV_MUTEX,
SND_DIED,
RCV_DIED,
RCV_CONNECT,
};
//===============================================================================================================
// Descriptions and prototypes (Code dependant)
//===============================================================================================================
#define PFS(semid, num) \
OUT1("[Line = %d] Printing flags...\n", __LINE__); \
for (int k = 0; k < num; ++k) \
{ \
OUT2("[%d] - %d\n", k, semctl(semid, k, GETVAL)); \
} \
OUT("\n"); \
struct Wait_for
{
int semid;
int num;
char* msg;
};
int send(const char* filename);
int receive();
int is_sender(int*);
int is_receiver(int*);
void* kill_if_died(void* ptr);
int snd_protect_connection(int semid);
int rcv_protect_connection(int semid);
int snd_clean(int semid, int fileid, int shmemid, int flagid);
int get_sems(const char* filename, int num);
void* get_memptr(const char* filename, size_t size, int* id_to_save);
int set_memory(const char* filename, size_t size, int* need_to_init);
//===============================================================================================================
//===============================================================================================================
#endif
/**
* @brief Safe sending file via shared memory and semaphors
*
* @details If the first argument is empty, program works as a receiver, otherwise it
* sends the argumented file
*
* @mainpage
*
* @author Sergey Ivanychev, DCAM MIPT, 376 group
*
* @version v. 1.02
*/
//===================================================================================
//===================================================================================
int main(int argc, char const *argv[])
{
if (argc > 1)
send(argv[1]);
else
receive();
}
//===================================================================================
//===================================================================================
void last_cleaner()
{
unlink(FILE_NAME_SHMEM_ATTACH);
unlink(SND_FLAG);
unlink(RCV_FLAG);
}
//===================================================================================
//===================================================================================
#undef F_CHECK_EXIT_CODE
#define F_CHECK_EXIT_CODE last_cleaner(); //! See header for description of F_CHECK_EXIT_CODE
int receive()
{
int flagid = 0;
int master_receiver = is_receiver(&flagid);
if (!master_receiver)
exit(EXIT_SUCCESS);
int semid = get_sems( FILE_NAME_SHMEM_ATTACH, 5);
CHECK(semid != -1, "Failed to get semaphors id");
int cond = rcv_protect_connection(semid);
CHECK(cond == 0, "Failed to set up protected connection");
int shmemid = 0;
void* smbuf = get_memptr( FILE_NAME_SHMEM_ATTACH,
BUF_SIZE + sizeof(long),
&shmemid);
CHECK(smbuf != (void*)-1, "Failed to get shared memory pointer");
struct sembuf mutexes[5] = {
{SND_MUTEX, -1, 0},
{RCV_MUTEX, 1, 0},
{SND_DIED, 0, IPC_NOWAIT},
{RCV_MUTEX, 0, 0},
{RCV_CONNECT,-1, 0},
};
long* nbytes_to_save = smbuf + BUF_SIZE;
PFS(semid, 5);
cond = semop(semid, &mutexes[4], 1);
CHECK(cond != -1, "Failed to connect to sender");
OUT("# Activated\n");
PFS(semid, 5);
cond = semop(semid, &mutexes[2], 2);
while (*nbytes_to_save != -1)
{
OUT1("# Printing %ld bytes\n", *nbytes_to_save);
cond = write(STDOUT_FILENO, smbuf, *nbytes_to_save);
if (cond != *nbytes_to_save)
fprintf(stderr, "Printed = %d\nExpected = %ld\n", cond, *nbytes_to_save);
CHECK(cond == *nbytes_to_save, "Printed ammount of bytes isn't valid");
OUT("# Getting...\n");
cond = semop(semid, &mutexes[0], 2);
cond = semop(semid, &mutexes[2], 2);
if (cond == -1)
{
printf("Sender dead\n");
goto RECEIVE_CLOSING;
}
OUT("# Got package\n");
CHECK(cond == 0, "Failed to set semaphors");
};
OUT("# Finishing receiving\n");
semop(semid, &mutexes[0], 1);
RECEIVE_CLOSING:
unlink(RCV_FLAG);
semctl(flagid, 0, IPC_RMID);
exit(EXIT_SUCCESS);
}
//===================================================================================
//===================================================================================
int send(const char* filename)
{
int flagid = 0;
int master_sender = is_sender(&flagid);
if (!master_sender)
exit(EXIT_SUCCESS);
int shmemid = 0;
void* smbuf = get_memptr(
FILE_NAME_SHMEM_ATTACH,
BUF_SIZE + sizeof(long),
&shmemid);
CHECK(smbuf != (void*)-1, "Failed to get shared memory pointer");
long* nbytes_to_save = smbuf + BUF_SIZE;
CHECK(smbuf != (void*)-1, "Failed to get memory pointer");
int semid = get_sems( FILE_NAME_SHMEM_ATTACH, 5);
CHECK(semid != -1, "Failed to get semaphors id");
int cond = snd_protect_connection(semid);
CHECK(cond == 0, "Failed to set up protected connection");
int fileid = open(filename, O_RDONLY);
CHECK(fileid != -1, "Failed to open file to send");
struct sembuf rcv_activate[3] = {
{RCV_MUTEX, 1, 0},
{RCV_CONNECT, 1, 0},
{RCV_CONNECT, 0, 0}
};
cond = semop(semid, rcv_activate, 2);
cond = semop(semid, &rcv_activate[2], 1);
CHECK(cond != -1, "Failed to connect to receiver");
struct sembuf mutexes[4] = {
{SND_MUTEX, 1, 0},
{RCV_MUTEX, -1, 0},
{RCV_DIED, 0, IPC_NOWAIT},
{SND_MUTEX, 0, 0}
};
int nbytes = 0;
while ((nbytes = read(fileid, smbuf, BUF_SIZE)) > 0)
{
*nbytes_to_save = nbytes;
OUT1("# Sending %ld bytes\n", *nbytes_to_save);
cond = semop(semid, &mutexes[0], 2);
cond = semop(semid, &mutexes[2], 2);
if (cond == -1)
{
printf("Receiver dead\n");
goto SENDER_CLOSING;
}
CHECK(cond == 0, "# Failed to set semaphors");
OUT("# Package sent\n");
}
OUT("# Finishing...\n");
*nbytes_to_save = -1;
cond = semop(semid, &mutexes[0], 2);
cond = semop(semid, &mutexes[2], 2);
if (cond == -1)
{
printf("Receiver dead\n");
goto SENDER_CLOSING;
}
OUT("# Finished!\n");
SENDER_CLOSING:
cond = snd_clean(semid, fileid, shmemid, flagid);
exit(EXIT_SUCCESS);
}
#undef F_CHECK_EXIT_CODE
#define F_CHECK_EXIT_CODE return -1;
//===================================================================================
//===================================================================================
int is_sender(int* semflag)
{
int fileid = open(SND_FLAG, O_CREAT, 0600);
CHECK(fileid != -1, "Failed to open sender flag");
int key = ftok(SND_FLAG,1);
CHECK(key != -1, "Failed to get flag key");
int semid = semget(key, 1, IPC_CREAT | 0660);
CHECK(semid != -1, "Failed to get flag semaphore id");
*semflag = semid;
struct sembuf act_flag[2] = {
{0, 0, IPC_NOWAIT},
{0, 1, SEM_UNDO}
};
int cond = semop(semid, act_flag, 2);
if (cond == -1 && errno != EAGAIN)
CHECK(0, "Unexpected error happened");
if (cond == -1)
return 0;
return 1;
}
int is_receiver(int* semflag)
{
int fileid = open(RCV_FLAG, O_CREAT, 0600);
CHECK(fileid != -1, "Failed to open receiver flag");
int key = ftok(RCV_FLAG, 1);
CHECK(key != -1, "Failed to get flag key");
int semid = semget(key, 1, IPC_CREAT | 0660);
CHECK(semid != -1, "Failed to get flag semaphore id");
*semflag = semid;
struct sembuf act_flag[2] = {
{0, 0, IPC_NOWAIT},
{0, 1, SEM_UNDO}
};
int cond = semop(semid, act_flag, 2);
if (cond == -1 && errno != EAGAIN)
CHECK(0, "Unexpected error happened");
if (cond == -1)
return 0;
return 1;
}
//===================================================================================
//===================================================================================
//===================================================================================
//===================================================================================
#undef F_CHECK_EXIT_CODE
#define F_CHECK_EXIT_CODE return -1;
int snd_protect_connection(int semid)
{
struct sembuf snd_ifdie = {
SND_DIED,
1,
0};
struct sembuf snd_ifdie2 = {
SND_DIED,
-1,
SEM_UNDO};
int cond = semop(semid, &snd_ifdie, 1);
CHECK(cond == 0, "Failed to set SND_IFDIE semaphor");
cond = semop(semid, &snd_ifdie2, 1);
CHECK(cond == 0, "Failed to set SND_IFDIE semaphor");
struct sembuf unlock_mutex_ifdead[2] = {
{RCV_MUTEX, 1, SEM_UNDO},
{RCV_MUTEX, -1, 0}
};
cond = semop(semid, unlock_mutex_ifdead, 2);
CHECK(cond == 0, "Failed to set RCV_MUTEX lock");
return 0;
}
int rcv_protect_connection(int semid)
{
struct sembuf sem_undo_zero[2] = {
{RCV_DIED, 1, 0},
{RCV_DIED, -1, SEM_UNDO}};
int cond = semop(semid, &sem_undo_zero[0], 1);
CHECK(cond == 0, "Failed to set RCV_DIED to undo mode");
cond = semop(semid, &sem_undo_zero[1], 1);
CHECK(cond == 0, "Failed to set RCV_DIED to undo mode");
struct sembuf unlock_mutex_ifdead[2] = {
{SND_MUTEX, 1, SEM_UNDO},
{SND_MUTEX, -1, 0}
};
cond = semop(semid, unlock_mutex_ifdead, 2);
CHECK(cond == 0, "Failed to set SND_MUTEX lock");
return 0;
}
#undef F_CHECK_EXIT_CODE
//===================================================================================
//===================================================================================
int snd_clean(int semid, int fileid, int shmemid, int flagid)
{
if (semid >= 0)
semctl(semid, 0, IPC_RMID);
if (fileid >= 0)
close(fileid);
if (shmemid >= 0)
shmctl(shmemid, IPC_RMID, NULL);
if (flagid >= 0)
semctl(flagid, 0, IPC_RMID);
unlink(SND_FLAG);
return 0;
}
//===================================================================================
//===================================================================================
#define F_CHECK_EXIT_CODE return -1;
int get_sems(const char* filename, int num)
{
CHECK(filename, "Argumented filename pointer is NULL");
int fileid = creat(filename, 660);
CHECK(fileid != -1 || (fileid == -1 && errno == EEXIST), "Unexpected error during sharing memory file creation");
key_t file_key = ftok(filename, 1);
CHECK(file_key != -1, "Failed to get System V key");
OUT3("# File key: %d\n# Number: %d\n# Flag: %o\n", file_key, num, IPC_CREAT | 0660);
int sem_id = semget(file_key, num, IPC_CREAT | IPC_EXCL | 0660);
if (sem_id == -1)
return semget(file_key, num, IPC_CREAT | 0660);
int cond = 0;
for (int i = 0; i < num; ++i)
{
cond = semctl(sem_id, i, SETVAL, 0);
CHECK(cond != -1, "Failed to initialize semaphor array");
}
return sem_id;
}
#undef F_CHECK_EXIT_CODE
//===================================================================================
//===================================================================================
#define F_CHECK_EXIT_CODE return (void*)-1;
void* get_memptr(const char* filename, size_t size, int* id_to_save)
{
CHECK(filename, "Argumented filename pointer is NULL");
CHECK(id_to_save,"Argumented pointer is NULL" );
WARN(size >= PAGESIZE, "Requested size of shared memory will be rounded up to PAGESIZE");
int cond = creat(filename, 0660);
if (cond == -1)
printf("Failed to create file [%s]\n", filename);
CHECK(cond != -1 || (cond == -1 && errno == EEXIST), "Unexpected error during sharing memory file creation");
int need_to_init = 0;
int shared_id = set_memory(filename,
size,
&need_to_init);
CHECK(shared_id != -1, "Failed to get shared memory");
void* shm_ptr = shmat(shared_id, NULL, 0);
OUT1("NBytes = [%ld]\n", *(long*)(shm_ptr + BUF_SIZE));
OUT1("# Shared memory pointer [%p]\n", shm_ptr);
CHECK(shm_ptr != (void*) -1, "Failed to get pointer from share memory id");
*id_to_save = shared_id;
if (need_to_init == 0)
return shm_ptr;
memset(shm_ptr, size, 0);
return shm_ptr;
}
//===================================================================================
//===================================================================================
#undef F_CHECK_EXIT_CODE
#define F_CHECK_EXIT_CODE return -1;
int set_memory(const char* filename, size_t size, int* need_to_init)
{
assert(filename);
assert(size);
key_t file_key = ftok(filename, 1);
CHECK(file_key != -1, "Failed to get System V key");
OUT2("# Shared memory creation key [%d], name [%s]\n", file_key, filename);
int memid = shmget(file_key, size, IPC_CREAT |IPC_EXCL| 0660);
if (memid == -1)
return shmget(file_key, size, IPC_CREAT | 0660);
*need_to_init = 1;
OUT1("# Shared memory ID [%d]\n", memid);
return memid;
}
#undef F_CHECK_EXIT_CODE
