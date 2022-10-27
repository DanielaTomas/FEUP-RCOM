// Application layer protocol implementation
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "application_layer.h"
#include "link_layer.h"
#include "string.h"
#include "utils.h"
#include "../include/alarm.h"
#include <signal.h>
#include <time.h>

extern int fdR;
extern int fdT;
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
  
} 

