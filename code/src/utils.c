#include <fcntl.h>
#include "link_layer.h"
#include "alarm.h"
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

extern int fdR;
extern int fdT;
extern int alarm_enabled;
extern int alarm_count;
unsigned char error_flag = 0;
unsigned char msgNum = 0;
int nFrames = 0;

////////////////////////////////////////////////
// EXTRA-FUNCTIONS
////////////////////////////////////////////////

unsigned char *parse_message(unsigned char *msg, off_t *index, int *packet_size, off_t file_size) {
  
  unsigned char *packet;

  if (*index + *packet_size > file_size) {
    *packet_size = file_size - *index;
  }

  packet = (unsigned char*)malloc(*packet_size);
  
  off_t j = *index;
  for (int i = 0; i < *packet_size; i++, j++) {
    packet[i] = msg[j];
  }

  *index = j;

  return packet;
}

unsigned char *remove_header(unsigned char *remove, int size_to_remove, int *size_removed) {

  unsigned char *msg = (unsigned char*)malloc(size_to_remove - 4);

  for (int i = 0, j = 4; i < size_to_remove; i++, j++) {
    msg[i] = remove[j];
  }

  *size_removed = size_to_remove - 4;

  return msg;
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
  
  if(write(fdT, packet, 5) == -1) {
    printf("Couldn't write (send_message)\n");
  }
}

unsigned char *control_packetI(unsigned char state, off_t file_size, unsigned char *file_name, int filename_size, int *packetI_size) {
  *packetI_size = 9 * sizeof(unsigned char) + filename_size;
  unsigned char *packet = (unsigned char *)malloc(*packetI_size);

  if (state != START2) {
    packet[0] = END2;
    packet[1] = CV0;
    packet[2] = CV6;
    packet[3] = (file_size >> 24) & 0xFF;
    packet[4] = (file_size >> 16) & 0xFF;
    packet[5] = (file_size >> 8) & 0xFF;
    packet[6] = file_size & 0xFF;
    packet[7] = HEADERC;
    packet[8] = filename_size;
  }
  else {
    packet[0] = START2;
  }
  
  int itr = 0;
  while(itr < filename_size) {
    packet[9 + itr] = file_name[itr];
    itr++;
  }   
  return packet;
}

int verify_BCC2(unsigned char *packet, int size) {
  unsigned char BCC2 = packet[0];
  
  for(int itr = 1; itr < size - 1; itr++) {
    BCC2 ^= packet[itr];
  }
  
  return BCC2 == packet[size-1];
}

unsigned char *header_app_level(unsigned char *packet, off_t file_size, int *headerSize){
  unsigned char *packetFinal = (unsigned char *)malloc(file_size + 4);
  packetFinal[0] = HEADERC;
  packetFinal[1] = msgNum % 255;
  packetFinal[2] = (int)file_size / 256;
  packetFinal[3] = (int)file_size % 256;
 
  memcpy(packetFinal + 4, packet, *headerSize);
 
  *headerSize += 4;
  msgNum++;
  nFrames++;

  return packetFinal;
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
    
      bytesWritten = write(fdT, copy, frame_size);

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
  } while((alarm_enabled && alarm_count < byte_max) || declined);

  return bytesWritten;
}