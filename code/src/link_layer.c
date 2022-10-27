#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "link_layer.h"
#include "state_machine.h"
#include "alarm.h"
#include "utils.h"

#define _POSIX_SOURCE 1 

struct termios oldtio;
struct termios newtio;

unsigned char buf[128], *giant_buf = NULL;
unsigned long giant_buf_size = 0;
unsigned char data_s_flag = 0;

int fd;
int disc = FALSE;
LinkLayer cp;

int llopen(LinkLayer connectionParameters) {

    cp = connectionParameters;

    // Open serial port device for reading and writing and not as control_valueling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(cp.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) {
        perror(cp.serialPort);
        exit(-1);
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("Error getting new port settings.\n");
        exit(-1);
    }

    // Clear struct for new port settings
    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = cp.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 1 char received

    if(tcflush(fd, TCIOFLUSH) == -1) {
        perror("Error flushing the data.\n");
        exit(-1);
    }

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("Error setting new port settings.\n");
        exit(-1);
    }
    
    signal(SIGALRM,alarmHandler);
    
    switch(cp.role) {
         case LlRx:
            return llopenR(fd);
            break;
         case LlTx:
            return llopenT(fd,cp.nRetransmissions,cp.timeout);
            break;
         default:
            break;
    } 

    return -1;  
}

int llwrite(const unsigned char *buffer, int bufferSize) {

}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
}
