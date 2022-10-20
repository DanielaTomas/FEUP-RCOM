#include <fcntl.h>
#include "link_layer.h"
#include "alarm.h"
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


unsigned char error_flag = 0;
////////////////////////////////////////////////
// EXTRA-FUNCTIONS
////////////////////////////////////////////////
unsigned char *remove_header(unsigned char *toRemove, int sizeToRemove, int *sizeRemoved)
{
  int i = 0;
  int j = 4;
  unsigned char *messageRemovedHeader = (unsigned char *)malloc(sizeToRemove - 4);
  for (; i < sizeToRemove; i++, j++)
  {
    messageRemovedHeader[i] = toRemove[j];
  }
  *sizeRemoved = sizeToRemove - 4;
  return messageRemovedHeader;
}

int msg_ended(unsigned char *packet, unsigned char *end, int packet_len, int end_len) {
  int begin = 1, finish = 1;
  
  if(packet_len == end_len) {
    if (end[0] == END2) {
      while(begin < packet_len) {
        if (packet[begin] != end[finish])
          return FALSE;
        begin++, finish++;
      }
      return TRUE;
    }
  }
  return FALSE;
}

void send_message(unsigned char CV) {
  unsigned char packet[5];
  
  packet[0] = F;
  packet[1] = A_T;
  packet[2] = CV;
  packet[3] = packet[1] ^ packet[2];
  packet[4] = F;
  
  if(write(getFileDescriptor(), packet, 5) == -1) {
    printf("Couldn't write (send_message)\n");
  }
}

int verify_BCC2(unsigned char *packet, int size) {
  unsigned char BCC2 = packet[0];
  
  for(int itr = 1; itr < size - 1; itr++) {
    BCC2 ^= packet[itr];
  }
  
  return BCC2 == packet[size-1];
}

int state_machine_read(RState *state, unsigned char reader, int keep_data, unsigned char byte, int frame_type, int size, unsigned char *packet) {
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
            if(byte == CV0) {
              frame_type = 0;
              reader = byte;
              *state = C_RCV;
            }
            else if (byte == CV1) {
                frame_type = 1;
                reader = byte;
                *state = C_RCV;
            }
            else if ( byte == F) {
                *state = FLAG_RCV;
            }
            else {
                *state = START;
            }
            break;

        case C_RCV:
            if (byte == (A_T ^ reader)) {
                *state = BCC_OK;
            }
            else {
                *state = START;
            }
            break;

        case BCC_OK:
            if( byte == F) {
              if(verify_BCC2(packet, size)) {
                if(frame_type == 0) {
                  send_message(CV4);
                }
                else {
                  send_message(CV5);
                }
                printf("Sended REJ, T:%d\n", frame_type);
                *state = BREAK;
                keep_data = FALSE;
              }
            }
            else if(byte == ESC) {
              *state = STOP;
            }
            else {
              packet = (unsigned char *)realloc(packet,++(size));
              packet[size-1] = byte; 
            }
            break;
        case STOP: 
            if(byte == ESC_F ) {
              packet = (unsigned char *)realloc(packet,++(size));
              packet[size-1] = F;
            }
            else if(byte == ESC_E) {
              packet = (unsigned char *)realloc(packet,++(size));
              packet[size-1] = ESC;
            }
            else {
              printf("Error after escape character\n");
              return -1; 
            }
            *state = BCC_OK;
            break;
    }
    return size;
}

unsigned char read_message() {
  RState state = START;
  unsigned char byte1, byte2;

  while (!isAlarmEnabled() && state != STOP) {
    read(getFileDescriptor(), &byte1, 1);
    switch (state){
    case START:
      if (byte1 == F) {
        state = FLAG_RCV;
      }
      break;
    case FLAG_RCV:
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
    }
  }
  return 0xFF;
}

int send_message_W(unsigned char* frame_msg, int frame_size, int frame_type) {

   int declined = FALSE;
   int bytesWritten = -1;

   do {

      unsigned char *copy = (unsigned char*)malloc(frame_size),msg;
      
      memcpy(copy, frame_msg, frame_size);
      int random = (rand() % 100) +1;
    
      //BCC1 MODIFIED
      if(random <= 0) {
        do {
          msg = (unsigned char) ('A' + (rand() % 256));
        } while(msg == copy[3]);

        copy[3] = msg;
            error_flag = 1;

        printf("BCC1 Modified\n");
      }
      //free(msg);

      //BCC2 MODIFIED
      if (random <= 0) {
          int i = (rand() % (frame_size - 5)) + 4;
          unsigned char random_msg = (unsigned char)('A' + (rand() % 26));
          copy[i] = random_msg;
          
          printf("BCC2 Modified\n");
      }
    
      bytesWritten = write(getFileDescriptor(), copy, frame_size);

      setAlarmEnabled(FALSE);
      alarm(4);
      unsigned char c = read_message();
      if ((c == CV3 && frame_type == 0) || (c == CV2 && frame_type == 1)) {
        declined = FALSE;
        setAlarmCount(0);
        frame_type ^= 1;
        alarm(0);
      }
      else {
        if (c == CV5 || c == CV4) {
          declined = TRUE;
          alarm(0);
        }
      }
  } while((isAlarmEnabled() && getAlarmCount() < byte_max) || declined);

  return bytesWritten;
}

void readControlMessage(unsigned char CV){
  unsigned char byte;
  RState state = START;
  while(state != STOP) {
    read(getFileDescriptor(),&byte,1);
    switch (state){
    case START:
      if(byte == F)
        state = FLAG_RCV;
      break;

    case FLAG_RCV:
        if(byte == A_RCV) {
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
      state = (byte == (A_RCV ^ CV)) ? BCC_OK : START;
      break;

    case BCC_OK:
      state = (byte == F) ? STOP : START;
      break; 
    }
  }
}
