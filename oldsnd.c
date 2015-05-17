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
    if (argc != 2) {
        printf ("ERROR: (Sender) incorrect number of arguments\n");
        return -1;
    }

//=============== TRYING_TO_START_WORK ======================================
    if (mkfifo ("fifo_snd", 0644) && errno != EEXIST) {
        printf ("ERROR: (Sender) mkfifo (fifo_snd) failed\n");
        return -1;
    }
    if (mkfifo ("fifo_rcv", 0644) && errno != EEXIST) {
        printf ("ERROR: (Sender) mkfifo (fifo_rcv) failed\n");
        return -1;
    }

    int fifo_rcv_d = open ("fifo_rcv", O_NONBLOCK, O_RDONLY);
    ASSERT_snd (fifo_rcv_d > 2,
                "ERROR: open (fifo_rcv) failed\n");
    int fifo_snd_d = open ("fifo_snd", O_WRONLY);
    ASSERT_snd (fifo_snd_d > 2,
                "ERROR: open (fifo_snd) failed\n");

    int fifo_snd_size = fcntl (fifo_snd_d, F_GETPIPE_SZ);
    void* buf_snd = calloc (fifo_snd_size, 1);
    ASSERT_snd (buf_snd,
                "ERROR: calloc (buf_snd) failed\n");
    fcntl (fifo_snd_d, F_SETFL, O_NONBLOCK);
    if (write (fifo_snd_d, buf_snd, fifo_snd_size) == -1) {
        DEBUG printf ("[%d] I lost the race of senders :(\n", getpid());
        return 0;
    }

//=============== SENDING_FILE_TO_RECIEVER ==================================
    if (mkfifo ("fifo", 0644) && errno != EEXIST) {
        printf ("ERROR: (Sender) mkfifo failed\n");
        return -1;
    }
    int fifo_d = open ("fifo", O_WRONLY);
    ASSERT_snd (fifo_d > 2,
                "ERROR: open (fifo) failed\n");
    int input_d = open (argv[1], O_RDONLY);
    ASSERT_snd (input_d > 2,
                "ERROR: open (argv[1]) failed\n");
    void* buf = calloc (BUF_SIZE, 1);
    ASSERT_snd (buf,
                "ERROR: calloc (buf) failed\n");

    int nBytes = BUF_SIZE;
    while (nBytes == BUF_SIZE) {
        nBytes = read (input_d, buf, BUF_SIZE);
        write (fifo_d, buf, nBytes);
    }

//=============== FINISHING_WORK ============================================
    close (input_d);
    free  (buf);
    close (fifo_snd_d);
    close (fifo_rcv_d);
    close (fifo_d);

    return 0;
}
