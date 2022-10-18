// State Machine header.
#include "link_layer.h"

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

void state_machine(LinkLayer connectionParameters, unsigned char byte, RState* state);

#endif // _STATE_MACHINE_H_