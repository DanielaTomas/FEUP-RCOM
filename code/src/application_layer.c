// Application layer protocol implementation
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include "application_layer.h"
#include "link_layer.h"
#include "string.h"
#include "../include/alarm.h"
#include <signal.h>
#include <time.h>

extern int fd;
extern int nFrames;

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {

  LinkLayer connectionParameters;
  strcpy(connectionParameters.serialPort,serialPort);

  if(strcmp(role,"rx") == 0) {
    connectionParameters.role = LlRx;
  }
  else if(strcmp(role,"tx") == 0) {
    connectionParameters.role = LlTx;
  }
  else {
      perror(role);
      exit(-1);
  }
  
  connectionParameters.baudRate = baudRate;
  connectionParameters.nRetransmissions = nTries;
  connectionParameters.timeout = timeout;
  
  if(connectionParameters.role == LlRx) {

    if(llopen(connectionParameters) == -1) {
      perror("Error opening a connection using the port parameters.");
    }

    int packet_size = 0;
    int buffSize = 0;
    unsigned char *packet;
    off_t file_size = 0;
    off_t index = 0;

    printf("Message Size: %d\n", llread(packet));

    //NFFS
    int L2 = (int)packet[8];
    unsigned char *file_name = (unsigned char *)malloc(L2 + 1);

    for (int i = 0; i < L2; i++) {
      file_name[i] = packet[9 + i];
    }

    file_name[L2] = '\0'; 

    //SFFS
    file_size = (packet[3] << 24) | (packet[4] << 16) | (packet[5] << 8) | (packet[6]);

    unsigned char *msg = (unsigned char *)malloc(file_size);

    unsigned char final_msg;
    while(TRUE) {
      buffSize = llread(&final_msg);
      if (buffSize == 0) {
        continue;
      }  
      if (msg_ended(&packet, final_msg, packet_size, buffSize)) {
        printf("Final message received\n");
        break;
      }

      int size_without_header = 0;

      final_msg = remove_header(final_msg, buffSize, &size_without_header);

      memcpy(msg + index, final_msg, size_without_header);
      index += size_without_header;
    }

    printf("Message: ");
    for(int i = 0; i < file_size; i++) {
      printf("%x", msg[i]);
    }
    printf("\n");

    FILE *file = fopen((char*)file_name,"wb+"); 
    //FILE *file = fopen((char*)filename,"wb+"); 

   /* fseek(file, 0, SEEK_END);
    int fileSize = (int)ftell(file);
    rewind(file);

    if (file_size != fileSize){
        perror("File size doesn't match\n");
        exit(-1);
    } */

    printf("File Size: %zd\n", file_size);

    fwrite((void*)msg, 1, file_size, file);

    printf("New file created\n");

    fclose(file);
    llclose(0);
    sleep(1);  
  }
  else {

    (void)signal(SIGALRM,alarmHandler);
    off_t file_size;
    off_t index = 0;
    int packetI_size = 0;

    FILE *file;
    struct stat data;

    if((file = fopen((char *)filename,"rb")) == NULL) {
      perror("Can't open the file\n");
      exit(-1);
    }    
    
    stat((char *)filename, &data);
    file_size = data.st_size;
    printf("File bytes: %ld\n", file_size);

    unsigned char *msg = (unsigned char *)malloc(file_size);

    fread(msg, sizeof(unsigned char), file_size, file);

    struct timespec start_request, end_request;
    clock_gettime(CLOCK_REALTIME, &start_request);

    if(llopen(connectionParameters) == -1) {
      perror("Error opening a connection using the port parameters.");
      exit(-1);
    }

    int filename_size = strlen(filename);
    unsigned char *file_name = (unsigned char*)malloc(filename_size);
    unsigned char *packet = control_packetI(START2, file_size, file_name, filename_size, &packetI_size);

    llwrite(packet, packetI_size);
   
    printf("Frame START sent.\n");

    int packet_size = 100; //100 bytes in each packet
    srand(time(NULL));

    while(index < file_size && packet_size == 100) {

      unsigned char *packet = parse_message(msg, &index, &packet_size, file_size);
      printf("Packet %d sent\n", nFrames);

      int header_size = packet_size;
      unsigned char *header_msg = header_app_level(packet, file_size, &header_size);
      
      printf("Send Message\n");
      if (llwrite(header_msg, header_size) == -1) {
        perror("Alarm limit reached\n");
        exit(-1);
      }
    }

    unsigned char *end = control_packetI(END2, file_size, file_name, filename_size, &packetI_size);
    llwrite(end,packetI_size);
    printf("Frame END sent\n");

    llclose(FALSE);

    clock_gettime(CLOCK_REALTIME, &end_request);

    double seconds = (end_request.tv_sec - start_request.tv_sec) + (end_request.tv_nsec - start_request.tv_nsec) / 1E9;

    printf("%f seconds\n", seconds);

    sleep(1);

    close(fd);
  }
} 

