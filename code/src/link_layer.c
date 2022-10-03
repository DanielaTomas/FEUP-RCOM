// Link layer protocol implementation

#include "link_layer.h"
#include <stdio.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopen(LinkLayer connectionParameters) {
    
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    struct termios oldtio, newtio;

    if (fd < 0) {
        perror(connectionParameters.serialPort);
        return -1;
    }

    if (tcgetattr(fd, &oldtio) == -1) {
        perror("Can't get the port settings.\n");
        return -1;
    }

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;

    if(tcflush(fd, TCIOFLUSH) == -1) {
        perror("Can't flush the data\n");
        return -1;
    }

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("Error setting termios struct.\n");
        return -1;
    }

    if(connectionParameters.role) {
        //llread();
    }
    else {
        //llwrite();   
    }

    return 1;
}

/*
int llopen(LinkLayer connectionParameters)
{
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) {
        perror(connectionParameters.serialPort);
        return -1;
    }

    struct termios oldter;
    struct termios newter;

    if (tcgetattr(fd, &oldter) == -1) {
        perror("Can't fetch port settings.\n");
        return -1;
    }

    memset(&newter, 0, sizeof(newter));

    newter.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newter.c_iflag = IGNPAR;
    newter.c_oflag = 0;

    newter.c_lflag = 0;
    newter.c_cc[VTIME] = 0;
    newter.c_cc[VMIN] = 1;

    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd, TCSANOW, &newter) == -1) {
        perror("Error setting termios struct.\n");
        exit(-1);
    }

    printf("New termios structure set\n");

    unsigned char sender_block[5];
    unsigned char receiver_block[5];
    
    sender_block_create(sender_block);
    receiver_block_create(receiver_block);
    
    write(fd, sender_block, 5);ll

    return 1;
}
*/
////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    return 1;
}
