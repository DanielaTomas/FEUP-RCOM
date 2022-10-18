// Application layer protocol implementation
#include <stdlib.h>
#include <stdio.h>
#include "application_layer.h"
#include "link_layer.h"
#include "string.h"

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

  if(llopen(connectionParameters) == -1) {
    perror("Error opening a connection using the port parameters.");
  }
  
}

