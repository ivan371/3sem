#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include "task_1.h"

int main (int argc, char** argv)
{
//=============== CHECKING_MAIN_ARGS ========================================
    if (argc != 1) {
        printf ("ERROR: (Reciever) incorrect number of arguments\n");
        return -1;
    }

//=============== TRYING_TO_START_WORK ======================================
    if (mkfifo ("fifo_rcv", 0644) && errno != EEXIST) {
        printf ("ERROR: (Reciever) mkfifo (fifo_snd) failed\n");
        return -1;
    }
    if (mkfifo ("fifo_snd", 0644) && errno != EEXIST) {
        printf ("ERROR: (Reciever) mkfifo (fifo_rcv) failed\n");
        return -1;
    }

    int fifo_snd_d = open ("fifo_snd", O_NONBLOCK, O_RDONLY);
    ASSERT_rcv (fifo_snd_d > 2,
                "ERROR: open (fifo_snd) failed\n");
    int fifo_rcv_d = open ("fifo_rcv", O_WRONLY);
    ASSERT_rcv (fifo_rcv_d > 2,
                "ERROR: open (fifo_rcv) failed\n");

    int fifo_rcv_size = fcntl (fifo_rcv_d, F_GETPIPE_SZ);
    void* buf_rcv = calloc (fifo_rcv_size, 1);
    ASSERT_rcv (buf_rcv,
                "ERROR: calloc (buf_rcv) failed\n");
    fcntl (fifo_rcv_d, F_SETFL, O_NONBLOCK);
    if (write (fifo_rcv_d, buf_rcv, fifo_rcv_size) == -1) {
        DEBUG printf ("[%d] I lost the race of recievers :(\n", getpid());
        return 0;
    }


//=============== GETTING_FILE_FROM_SENDER ==================================
    if (mkfifo ("fifo", 0644) && errno != EEXIST) {
        printf ("ERROR: (Reciever) mkfifo failed\n");
        return -1;
    }

    int fifo_d = open ("fifo", O_RDONLY);
    ASSERT_rcv (fifo_d > 2,
                "ERROR: open (fifo) failed\n");

    void* buf = calloc (BUF_SIZE, 1);
        if (!buf) {
            printf ("ERROR: calloc (buf) failed\n");
            return -1;
        }

    int nBytes = BUF_SIZE;
    while (nBytes != 0) {
        nBytes = read (fifo_d, buf, BUF_SIZE);
        write (STDOUT_FILENO, buf, nBytes);
    }

//=============== FINISHING_WORK ============================================
    unlink ("fifo_rcv");
    unlink ("fifo_snd");
    unlink ("fifo");
    free  (buf);
    close (fifo_snd_d);
    close (fifo_rcv_d);
    close (fifo_d);

    return 0;
}
