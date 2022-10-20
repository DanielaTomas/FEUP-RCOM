// Application layer protocol implementation
#include <stdlib.h>
#include <stdio.h>
#include "application_layer.h"
#include "link_layer.h"
#include "string.h"
#include "../include/alarm.h"
#include <signal.h>
#include <time.h>

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

    int start_len = 0;
    int buffSize = 0;
    unsigned char *frame_msg;
    unsigned char *packet;
    off_t msg_size = 0;
    unsigned char *msg; //gia
    off_t index = 0;

    printf("Message Size: %d\n", llread(&packet));

    //nameOfFFS
    int L2 = (int)packet[8];
    unsigned char *msg_name = (unsigned char *)malloc(L2 + 1);

    for (int i = 0; i < L2; i++) {
      msg_name[i] = packet[9 + i];
    }

    msg_name[L2] = '\0';

    //sizeOfFFS
    msg_size = (packet[3] << 24) | (packet[4] << 16) | (packet[5] << 8) | (packet[6]);

    msg = (unsigned char *)malloc(msg_size);

    while(TRUE) {
      frame_msg = llread(&buffSize);
      if (buffSize == 0) {
        continue;
      }  
      if (msg_ended(packet, start_len, frame_msg, buffSize)) {
        printf("Final message received\n");
        break;
      }

      int size_without_header = 0;

      frame_msg = remove_header(frame_msg, buffSize, &size_without_header);

      memcpy(msg + index, frame_msg, size_without_header);
      index += size_without_header;
    }

    printf("Message: \n");

    for(int i = 0; i < msg_size; i++) {
      printf("%x", msg[i]);
    }
    printf("\n");

    FILE *file = fopen((char*)msg_name,"wb+"); 

    fwrite((void *)msg, 1, msg_size, file);
    printf("%zd\n", msg_size);
    printf("New file created\n");
    fclose(file);
    llclose(0);

    sleep(1);  

  }
  else {
   /* (void)signal(SIGALRM, alarmHandler);
    off_t size_file;

    unsigned char *msg_file = ((unsigned char *)role, &size_file);

    //inicio do relógio
    struct timespec requestStart, requestEnd;
    clock_gettime(CLOCK_REALTIME, &requestStart);

    //se nao conseguirmos efetuar a ligaçao atraves do set e do ua o programa termina
    if(llopen(connectionParameters) == -1) {
      perror("Error opening a connection using the port parameters.");
    }

    int sizeOfFileName = strlen(argv[2]);
    unsigned char *fileName = (unsigned char *)malloc(sizeOfFileName);
    fileName = (unsigned char *)argv[2];
    unsigned char *start = controlPackageI(C2Start, sizeFile, fileName, sizeOfFileName, &sizeControlPackageI);

    LLWRITE(start, sizeControlPackageI);
    printf("Mandou trama START\n");

    int sizePacket = sizePacketConst;
    srand(time(NULL));

    while (sizePacket == sizePacketConst && indice < sizeFile) {
      //split mensagem
      unsigned char *packet = splitMessage(mensagem, &indice, &sizePacket, sizeFile);
      printf("Mandou packet numero %d\n", numTotalTramas);
      //header nivel aplicação
      int headerSize = sizePacket;
      unsigned char *mensagemHeader = headerAL(packet, sizeFile, &headerSize);
      //envia a mensagem
      if (!LLWRITE(fd, mensagemHeader, headerSize)) {
        printf("Limite de alarmes atingido\n");
        return -1;
      }
    }

    unsigned char *end = controlPackageI(END2, sizeFile, fileName, sizeOfFileName, &sizeControlPackageI);
    LLWRITE(fd, end, sizeControlPackageI);
    printf("Mandou trama END\n");

    LLCLOSE(fd);

    //fim do relógio
    clock_gettime(CLOCK_REALTIME, &requestEnd);

    double accum = (requestEnd.tv_sec - requestStart.tv_sec) + (requestEnd.tv_nsec - requestStart.tv_nsec) / 1E9;

    printf("Seconds passed: %f\n", accum);

    sleep(1);

    close(fd);*/
  }
} 

