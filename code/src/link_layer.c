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
extern int alarm_enabled;
struct termios oldtio, newtio;
LinkLayerRole role;
int frame_type = 0;
int fdT;
int fdR;
int waited = 0;

////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////

void llopenR(int fd, LinkLayer connectionParameters) {
    read_message_R(SETUP);
    printf("Sent SET\n");
    send_message(fd, UA);
    printf("Sent UA\n");
}

int llopenT(int fd, LinkLayer connectionParameters) {

  unsigned char byte;
  int stop = FALSE;

  do {
    send_message(fd, SETUP);
    alarm(connectionParameters.timeout);
    alarm_enabled = FALSE;
    RState state = START;

    while (!stop && !alarm_enabled) {
      read(fd, &byte, 1);
      state_machine_UA(&state, &byte, &stop);
    }

  } while (alarm_enabled && alarm_count < connectionParameters.nRetransmissions);

  if (!alarm_enabled && alarm_count != connectionParameters.nRetransmissions) {
    alarm_enabled = FALSE;
    alarm_count = 0;
    return 1;
  }

  //printf("Estou aqui!!");

  return -1;
}

int llopen(LinkLayer connectionParameters) {

    //Open serial port device for reading and writing and not as controlling tty because we don't want to get killed if linenoise sends CTRL-C.
    int fd;

    if(connectionParameters.role == LlRx) {
        fdR = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
        fd = fdR;
    }
    else {
        fdT = open(connectionParameters.serialPort, O_RDWR | O_NOCTTY);
        fd = fdT;
    }
  
    if (fd < 0) {
        perror(connectionParameters.serialPort);
        exit(-1);
    }
    
    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("Error getting the port settings.\n");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1; // Blocking read until 1 char received

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
            llopenR(fd, connectionParameters); // Lê a trama de controlo SET e envia a trama UA
            break;
            
        case LlTx:
            printf("LlTx\n");
            llopenT(fd, connectionParameters); // Envia trama de supervisão SET e recebe a trama UA
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
        printf("%d\n",bufSize);
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
// Lê as tramas I e a seguir faz destuffing
int llread(unsigned char *packet) {
  int size = 0;
  RState state = START; 
  frame_type = 0;
  unsigned char reader;
  int keep_data = 0;
  unsigned char byte;

  while(state != BREAK) {
    if(read(fdR,&byte,1) == -1) {
      perror("Couldn't read (llread)\n");
      exit(-1);
    }
    //printf("Byte: %x\n",byte);
    size = state_machine_R(&state, &reader, &keep_data, byte, &frame_type, size, packet);

    //printf("Packet Size: %d\n",size);
  }
  
  packet = (unsigned char *)realloc(packet, size - 1);
  size -= 1;
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

  // Lê a trama de controlo DISC, envia-a e recebe UA.
  if(role == LlRx) {
    read_message_R(DISC);
    printf("Received DISC\n");
    send_message(fdR, DISC);
    printf("Sended DISC\n");
    send_message(fdR, UA);
    printf("Received UA\n");
    printf("Terminated\n");
    
    if (tcsetattr(fdR, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
  }
  // Envia a trama de supervisão DISC, recebe-a e envia UA.
  else if(role == LlTx) {
    send_message(fdT, DISC);
    printf("Sended DISC\n");
    unsigned char CV;

    do {
        CV = read_message_T();
    } while (CV != DISC);

    printf("Read DISC\n");
    send_message(fdT, UA);
    printf("Sended UA final\n");
    printf("Writer terminated \n");

    if (tcsetattr(fdT, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
  }
  return 1;
}
