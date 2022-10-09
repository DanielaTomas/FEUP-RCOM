// Link layer protocol implementation
#include <fcntl.h>
#include "link_layer.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <termios.h>
#include <string.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

// FRAME
#define F 0x7E
#define A_T 0x03
#define A_R 0x01
#define SETUP 0x03
#define UA 0x07
#define BUFFER_SIZE 255
#define OK 0x00

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
void stateHandler(LinkLayer connectionParameters, unsigned char byte, RState* state, int frame[], int* frame_size) {
    switch (*state) {
        case START:
            if (byte == F) 
                *state = FLAG_RCV;
            break;

        case FLAG_RCV:
            if (byte == A_T) {
                *state = A_RCV;
            }
            else if (byte != F) {
                 *state = START;
            }
            break;

        case A_RCV:
            if (byte == F) {
                *state = FLAG_RCV;
            }
            else if ((byte == SETUP && connectionParameters.role == LlRx) 
                    || (byte == UA && connectionParameters.role == LlTx)) {
                *state = C_RCV;
            }
            else {
                *state = START;
            }
            break;

        case C_RCV:
            if (byte == F) {
                *state = FLAG_RCV;
            }
            else if ((byte == (A_T ^ SETUP) && connectionParameters.role == LlRx) 
                    || (byte == (A_T ^ UA) && connectionParameters.role == LlTx)) {
                *state = BCC_OK;
            }
            else {
                *state = START;
            }
            break;

        case BCC_OK:
            *state = (byte == F) ? STOP : START;
            break;
            
        default:
            break;
    }
}

int llopenR(LinkLayer connectionParameters, int fd) {
    RState state = START;

    printf("Loop for input \n");
    unsigned char byte;
    unsigned char frame[5];
    int frame_size = 0;

    while(state != STOP) {
        
        //printf("Role before read: %u\n", connectionParameters.role);
        
        if (read(fd, &byte, 1) == -1){
            //printf(connectionParameters.role);
            perror("Error reading a byte.\n");
            exit(-1);
        }
        frame_size++;
        //printf("Frame_size: %d\n",frame_size);
        //printf("Byte: %x\n", byte);
    
        stateHandler(connectionParameters, byte, &state, frame, &frame_size);
    }
    
    unsigned char ua[5] = {F, A_T, UA, A_T ^ UA, F};

    if(write(fd, ua, 5) == -1){
        perror("Error writting.\n");
        exit(-1);
    }
    return 1; 
}

llopenT(LinkLayer connectionParameters, int fd) {
    
}

int llopen(LinkLayer connectionParameters) {
    
    int fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
    struct termios oldtio, newtio;

    if (fd < 0) {
        perror(connectionParameters.serialPort);
        exit(-1);
    }

    if (tcgetattr(fd, &oldtio) == -1) {
        perror("Error getting the port settings.\n");
        exit(-1);
    }

    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;
    newtio.c_cc[VMIN] = 1;

    if (tcflush(fd, TCIOFLUSH) == -1) {
        perror("Error flushing the data.\n");
        exit(-1);
    }

    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("Error setting new port settings.\n");
        exit(-1);
    }

    printf("New termios structure set.\n");

    switch(connectionParameters.role) {
        case LlRx:
            llopenR(connectionParameters,fd);     
            break;
            
        case LlTx: 
            llopenT(connectionParameters,fd);
            break;
            
        default:
            break;
    }

    return 1;
}

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
