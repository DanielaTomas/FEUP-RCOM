// State Machine header.
#include "link_layer.h"

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

/*
 Retorna o tamanho
*/
int state_machine_R(RState *state, unsigned char* reader, int* keep_data, unsigned char byte, int* frame_type, int size, unsigned char *packet);
/*
  Retorna o campo de controlo da frame de Supervisao lida 
*/
unsigned char read_message_T();
/*
  Verifica se o frame recebido e o desejado
*/
void send_message_R(unsigned char CV);

void state_machine_UA(RState *state, unsigned char *byte, int *stop);

#endif // _STATE_MACHINE_H_