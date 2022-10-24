#include <fcntl.h>
#include "link_layer.h"
#include "alarm.h"
#include "utils.h"
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

extern int fdR;
extern int fdT;
extern int alarm_enabled;

void state_machine_UA(RState *state, unsigned char *byte, int *stop) {
  switch(*state) {
    
    case START:
     if(*byte == F) {
      *state = FLAG_RCV;
     }
     break;
     case FLAG_RCV:
      if (*byte == A_T) {
        *state = A_RCV;
      }
      else {
        if (*byte == F) {
          *state = FLAG_RCV;
        }
        else {
          *state = START;
        }
      }
      break;

    case A_RCV:
    if (*byte == UA) {
      *state = C_RCV;
    }
    else {
      if (*byte == F) {
        *state = FLAG_RCV;
      }
      else {
        *state = START;
      }
    }
    break;

    case C_RCV:
      if (*byte == (A_T ^ UA)) {
        *state = BCC_OK;
      }
      else {
        *state = START;
      }
      break;

    case BCC_OK:
      if (*byte == F) {
        *stop = TRUE;
        alarm(0);
        printf("Received UA\n");
      }
      else
        *state = START;
      break;

    default:
      break;
    }

} 

int state_machine_R(RState *state, unsigned char* reader, int* keep_data, unsigned char byte, int* frame_type, int size, unsigned char *packet) {
 switch (*state) {
        case START:
            printf("State 0\n");
            if (byte == F) {
               *state = FLAG_RCV;
            }
            break;

        case FLAG_RCV:
            printf("State 1\n");
            if (byte == A_T) {
              *state = A_RCV;
            }
            else {
              *state = (byte == F) ? FLAG_RCV : START;
            }
            break;

        case A_RCV:
            printf("State 2\n");
            if(byte == CV0) {
              *frame_type = 0;
              *reader = byte;
              *state = C_RCV;
            }
            else if (byte == CV1) {
              *frame_type = 1;
              *reader = byte;
              *state = C_RCV;
            }
            else{
              *state = (byte == F)? FLAG_RCV : START;
            } 
            break;

        case C_RCV:
            printf("State 3\n");
            if (byte == (A_T ^ *reader)) {
                *state = BCC_OK;
            }
            else {
                *state = START;
            }
            break;

        case BCC_OK:
            printf("State 4\n");
            if(byte == F) {
              if(verify_BCC2(packet, size)) {
                //printf("bcc2 entrei \n");
                if(*frame_type == 0) {
                  send_message(fdR, CV3);
                }
                else {
                  send_message(fdR, CV2);
                }
                printf("Sent RR\n");
                *state = BREAK;
                *keep_data = TRUE;
              }
              else {
                if (*frame_type == 0)
                  send_message(fdR, CV5);
                else
                  send_message(fdR, CV4);
                *state = BREAK;
                *keep_data = FALSE;
                printf("Sent REJ\n");
              }
            }
            else if(byte == ESC) {
              *state = STOP;
            }
            else {
              packet = (unsigned char *)realloc(packet,(++size));
              packet[size-1] = byte; 
            }
            break;

        case STOP: 
            printf("State 5\n");
            if(byte == ESC_F ) {
              packet = (unsigned char *)realloc(packet,(++size));
              packet[size-1] = F;
            }
            else if(byte == ESC_E) {
              packet = (unsigned char *)realloc(packet,(++size));
              packet[size-1] = ESC;
            }
            else {
              perror("Error after escape character\n");
              exit(-1); 
            }
            *state = BCC_OK;
            break;
            
        default:
            break;
    }
    return size;
}

// Espera ler o DISC
unsigned char read_message_T() {
  RState state = START;
  unsigned char byte1, byte2;

  while (!alarm_enabled && state != STOP) {
    read(fdR, &byte1, 1);
    switch (state){
    case START:
      if (byte1 == F) {
        printf("flag\n");
        state = FLAG_RCV;
      }
      break;
    case FLAG_RCV:
      printf("rcv\n");
      if (byte1 == A_T) {
        state = A_RCV;
      }
      else if (byte1 == F) {
          state = FLAG_RCV;
      }    
      else {
          state = START;
      }
      break;
      
    case A_RCV:
      if (byte1 == CV2 || byte1 == CV3 || byte1 == CV4 || byte1 == CV5 || byte1 == DISC) {
        byte2 = byte1;
        state = C_RCV;
      }
      else {
        if (byte1 == F) {
          state = FLAG_RCV;
        }  
        else { 
          state = START;
        }     
      }
      break;

    case C_RCV:
      if (byte1 == (A_T ^ byte2)) {
        state = BCC_OK;
      }  
      else {
        state = START;
      }  
      break;

    case BCC_OK:
      if (byte1 == F){
        alarm(0);
        state = STOP;
        return byte2;
      }
      else {
        state = START;
      }
      break;

      default:
        break;
    }
  }
  return 0xFF;
}

//Setup da trama de supervisão
void send_message_R(unsigned char CV){
  unsigned char byte;
  RState state = START;
  while(state != STOP) {
    
    read(fdR,&byte,1);
    
    switch (state){
      
    case START:
      printf("START -> %x\n",byte);
      if(byte == F)
        state = FLAG_RCV;
      break;

    case FLAG_RCV:
        printf("FLAG_RCV: byte - %x \n",byte);
        if(byte == A_T) {
            state = A_RCV;
        }
        else if(byte == F) {
            state = FLAG_RCV;
        }
        else {
            state = START;
        }
        break;
        
    case A_RCV:
      printf("A_RCV: byte - %x \n",byte);
      if(byte == CV) {
        state = C_RCV;
      }
      else if (byte == F) {
        state = FLAG_RCV;
      }
      else {
        state = START;
      }
      break;
      
    case C_RCV:
      printf("C_RCV: byte - %x \n",byte);
      state = (byte == (A_T ^ CV)) ? BCC_OK : START;
      break;

    case BCC_OK:
      printf("BCC_OK: byte - %x \n",byte);
      state = (byte == F) ? STOP : START;
      break; 

    default:
      break;
    }
  }
}
