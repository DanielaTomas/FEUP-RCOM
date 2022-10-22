// Link layer protocol implementation
#include <fcntl.h>
#include "link_layer.h"
#include "alarm.h"
#include "utils.h"
#include "state_machine.h"
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

extern int alarm_count;
struct termios oldtio, newtio;
LinkLayerRole role;
int frame_type = 0;
int fd;
int waited = 0;
////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

int llopenR(LinkLayer connectionParameters) {
    //printf("llopenR\n");
    RState state = START;
    unsigned char byte;
    //unsigned char frame[5];
    //int frame_size = 0;

    while(state != STOP) {
        
        if (read(fd, &byte, 1) == -1){
            perror("Error reading a byte (llopenR)\n");
            exit(-1);
        }

        //frame_size++;
        //printf("Frame_size: %d\n",frame_size);
        printf("Byte: %x\n", byte);
    
        state_machine(connectionParameters, byte, &state);
    }
    
    unsigned char ua[5] = {F, A_T, UA, A_T ^ UA, F};

    if(write(fd, ua, 5) == -1){
        perror("Error writting (llopenR)\n");
        exit(-1);
    }
    return 1; 
}

int llopenT(LinkLayer connectionParameters) {
    //printf("Entered Tx \n");
    RState state = START;
    
    unsigned char set[5] = {F, A_T, SETUP, A_T ^ SETUP, F};
    unsigned char byte;
    
    (void)signal(SIGALRM, alarmHandler);
    while(alarm_count < connectionParameters.nRetransmissions) {
        
        if(write(fd, set, 5) == -1){
            perror("Error writting (llopenT)\n");
            exit(-1);
        }

        alarm(connectionParameters.timeout);

        while(state != STOP) {
            if (read(fd, &byte, 1) == -1){
                perror("Error reading a byte (llopenT)\n");
                exit(-1);
            }
            printf("%d\n",state);
            state_machine(connectionParameters, byte, &state);
        }
    }

    printf("ESTOU AQUI!!");
    
    return 1;
}

int llopen(LinkLayer connectionParameters) {
    
    fd = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
  

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

    role = connectionParameters.role;
    switch(connectionParameters.role) {
        case LlRx:
            printf("LlRx\n");
            llopenR(connectionParameters);     
            break;
            
        case LlTx:
            printf("LlTx\n");
            llopenT(connectionParameters);
            break;
            
        default:
            break;
    }
    
    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {

    if(bufSize < 0) {
        printf("%d",bufSize);
        return -1;
    }

    unsigned char bcc2 = buf[0];

    for (int i = 1; i < bufSize; i++)
        bcc2 ^= buf[i];

    unsigned char *bcc2_stuffed = (unsigned char*)malloc(sizeof(unsigned char));
    int bcc2_size = 1;
    int frame_size = bufSize + 6;

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
        frame_msg = (unsigned char*)realloc(frame_msg, ++frame_size);
        frame_msg[j] = ESC;
        frame_msg[j + 1] = ESC_F;
        j = j + 2;
      }
      else if(buf[i] == ESC) {
          frame_msg = (unsigned char*)realloc(frame_msg, ++frame_size);
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

    if (bcc2_size != 1) {
      frame_msg = (unsigned char*)realloc(frame_msg, ++frame_size);
      frame_msg[j] = bcc2_stuffed[0];
      frame_msg[j + 1] = bcc2_stuffed[1];
      j++;
    }  
    else {
      frame_msg[j] = bcc2;
    }
    frame_msg[j + 1] = F;

   int bytesWritten = send_message_W(frame_msg, frame_size, frame_type);
  
   if (bytesWritten-6 < 0 || alarm_count >= byte_max) {
       return -1;
   }

   return bytesWritten - 6;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {

  int size = 0;
  RState state = START; 
  frame_type = 0;
  unsigned char reader;
  int keep_data = 0;
  unsigned char byte;

  while(state != BREAK) {
    if(read(fd,&byte,1) == -1) {
      perror("Couldn't read (llread)\n");
      exit(-1);
    }
    //printf("Byte %x\n",byte);
    size = state_machine_R(&state, &reader, &keep_data, byte, &frame_type, size, packet);
   
    //printf("Packet Size: %d\n",*size);
  }

  if(keep_data && frame_type ==  waited) {
    waited ^= 1;
  }
  else {
    size = 0;
  }

  return size;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics) {

  if(role == LlRx) {
    readControlMessage(DISC);
    printf("Received DISC\n");
    send_message(DISC);
    printf("Sended DISC\n");
    send_message(UA);
    printf("Received UA\n");
    printf("Terminated\n");
    
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
  }

  else if(role == LlTx) {
    send_message( DISC);
    printf("Mandou DISC\n");
    unsigned char CV;
    CV = read_message();

    while (CV != DISC) {
        CV = read_message();
    }

    printf("Leu DISC\n");
    send_message(UA);
    printf("Mandou UA final\n");
    printf("Writer terminated \n");

    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
  }
  return 1;
}
