// Link layer protocol implementation
#include <fcntl.h>
#include "link_layer.h"
#include "alarm.h"
#include "main.c"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
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
int frame_type = 0;
#define F 0x7E
#define A_T 0x03
#define A_R 0x01
#define SETUP 0x03
#define UA 0x07
#define BUFFER_SIZE 255
#define OK 0x00

#define ESC 0x7D
#define ESC_F 0x5E
#define ESC_E 0x5D

//Control Values 
#define CV0 0x00
#define CV1 0x40
#define CV2 0x05
#define CV3 0x85
#define CV4 0x01
#define CV5 0x81

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
void stateHandler(LinkLayer connectionParameters, unsigned char byte, RState* state) {
    switch (*state) {
        case START:
            if (byte == F) {
                *state = FLAG_RCV;
            }
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
    //printf("llopenR\n");
    unsigned char byte;
    unsigned char frame[5];
    int frame_size = 0;

    while(state != STOP) {
        
        //printf("Role before read: %u\n", connectionParameters.role);
        
        if (read(fd, &byte, 5) == -1){
            //printf(connectionParameters.role);
            perror("Error reading a byte (llopenR)\n");
            exit(-1);
        }

        frame_size++;
        //printf("Frame_size: %d\n",frame_size);
        //printf("Byte: %x\n", byte);
    
        stateHandler(connectionParameters, byte, &state);
    }
    
    unsigned char ua[5] = {F, A_T, UA, A_T ^ UA, F};

    if(write(fd, ua, 5) == -1){
        perror("Error writting (llopenR)\n");
        exit(-1);
    }
    
    return 1; 
}

int llopenT(LinkLayer connectionParameters, int fd) {
    //printf("Entered Tx \n");
    RState state = START;
    
    unsigned char set[5] = {F, A_T, SETUP, A_T ^ SETUP, F};
    unsigned char byte;
    
    (void)signal(SIGALRM, alarmHandler);
    while(getAlarmCount() < connectionParameters.nRetransmissions) {
        
        if(write(fd, set, 5) == -1){
            perror("Error writting (llopenT)\n");
            exit(-1);
        }

        alarm(connectionParameters.timeout);

        while(state != STOP) {
            printf("Estoiro\n");
            if (read(fd, &byte, 1) == -1){
                perror("Error reading a byte (llopenT)\n");
                exit(-1);
            }
            stateHandler(connectionParameters, byte, &state);
        }
    
    }

    return 1;
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
            printf("LlRx\n");
            llopenR(connectionParameters,fd);     
            break;
            
        case LlTx:
            printf("LlTx\n");
            llopenT(connectionParameters,fd);
            break;
            
        default:
            break;
    }

    return 1;
}

////////////////////////////////////////////////
// EXTRA-FUNCTIONS
////////////////////////////////////////////////

/*send_message(unsigned char* frame_msg, int frame_size, int fd, int declined) {
   /*do {

    unsigned char *copy;
    
    //copy = messUpBCC1(frame_msg, frame_size); //altera bcc1
   /*copy = messUpBCC2(copy, frame_size);         //altera bcc2
    write(fd, copy, frame_size);

    alarm_enabled = FALSE;
    alarm(TIMEOUT);
    unsigned char C = readControlMessageC(fd);
    if ((C == CV3 && frame_type == 0) || (C == CV2 && frame_type == 1))
    {
      printf("Recebeu rr %x, frame = %d\n", C, frame_type);
      declined = FALSE;
      alarm_count = 0;
      frame_type ^= 1;
      alarm(0);
    }
    else
    {
      if (C == CV5|| C == CV4)
      {
        declined = TRUE;
        printf("Recebeu rej %x, frame=%d\n", C, frame_type);
        alarm(0);
      }
    }
  } while ((alarm_enabled && alarm_count < NUMMAX) || declined);
}*/

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(int fd, const unsigned char *buf, int bufSize) {

    if(bufSize < 0) {
        perror(bufSize);
        exit(-1);
    }

    unsigned char bcc2 = buf[0];
    int declined;

    for (int i = 1; i < bufSize; i++)
        bcc2 ^= buf[i];

    unsigned char *bcc2_stuffed = (unsigned char*)malloc(sizeof(unsigned char));
    int bcc2_size = 1;
    int frame_size = bufSize + 6;
    int rejected = FALSE;

    //Stuffing BCC2
    if (bcc2 == F) {
        bcc2_stuffed = (unsigned char*)malloc(2*sizeof(unsigned char*));
        bcc2_stuffed[0] = ESC;
        bcc2_stuffed[1] = ESC_F;
        bcc2_size++;
    }
    else if (bcc2 == ESC) {
        bcc2_stuffed = (unsigned char*)malloc(2*sizeof(unsigned char*));
        bcc2_stuffed[0] = ESC;
        bcc2_stuffed[1] = ESC_E;
        bcc2_size++;
    }

    unsigned char* frame_msg = (unsigned char*)malloc((bufSize+6)*sizeof(unsigned char));
    frame_msg[0] = F;
    frame_msg[1] = A_T;

    if(frame_type != 0) {
        frame_msg[2] = CV1;
    }
    else {
        frame_msg[2] = CV0;
    }
    frame_msg[3] = (frame_msg[1]^frame_msg[2]);

    int j = 4;
    int i = 0;
    while (i < bufSize) {
      if (buf[i] == F) {
        frame_msg = (unsigned char *)realloc(frame_msg, ++frame_size);
        frame_msg[j] = ESC;
        frame_msg[j + 1] = ESC_F;
        j = j + 2;
      }
      else if(buf[i] == ESC){
          frame_msg = (unsigned char *)realloc(frame_msg, ++frame_size);
          frame_msg[j] = ESC;
          frame_msg[j + 1] = ESC_E;
          j = j + 2;
      }
      else {
          frame_msg[j] = buf[i];
          j++;
      }     
      i++;
    }

    if (bcc2_size != 1){
      frame_msg = (unsigned char *)realloc(frame_msg, ++frame_size);
      frame_msg[j] = bcc2_stuffed[0];
      frame_msg[j + 1] = bcc2_stuffed[1];
      j++;
    }  
    else {
      frame_msg[j] = bcc2;
    }
    frame_msg[j + 1] = F;

   //send_message(frame_msg, frame_size, fd, declined);
  /*
  
  if (sumAlarms >= NUMMAX)
    return FALSE;
  else
    return TRUE; */

    return 1;
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
    // TODO medir tempos de envio obtendo S = R/C??
    return 1;
}
