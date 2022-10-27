// State Machine header.
#include "link_layer.h"

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_


extern unsigned char address;
extern unsigned char control_value;
extern unsigned char bcc;
extern unsigned char *data;
extern unsigned int size;

typedef enum {START, FLAG, ADDRESS, CONTROL, BCC1_OK, SUCCESS, ESCAPE, BCC2_OK, END, REJECT} FState;
extern FState fstate;

void state_machine(unsigned char byte);

#endif // _STATE_MACHINE_H_