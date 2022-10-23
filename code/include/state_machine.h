// State Machine header.
#include "link_layer.h"

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

void state_machine(LinkLayer connectionParameters, unsigned char byte, RState* state);

int state_machine_R(RState *state, unsigned char* reader, int* keep_data, unsigned char byte, int* frame_type, int size, unsigned char *packet);
/*
  Retorna o campo de controlo da frame de Supervisao lida 
*/
unsigned char read_message();
/*
  Verifica se o frame recebido e o desejado
*/
void readControlMessage(unsigned char CV);

#endif // _STATE_MACHINE_H_