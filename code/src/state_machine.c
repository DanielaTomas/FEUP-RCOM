#include <fcntl.h>
#include "link_layer.h"
#include "alarm.h"
#include "utils.h"
#include "main.c"
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

void state_machine(LinkLayer connectionParameters, unsigned char byte, RState* state) {
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
            if (byte == F) {
                *state = FLAG_RCV;
            }
            else if ((byte == SETUP && connectionParameters.role == LlRx) 
                    || (byte == UA && connectionParameters.role == LlTx)) {
                *state = C_RCV;
            }
            else {
                *state = START;
            }
            break;

        case C_RCV:
            if (byte == F) {
                *state = FLAG_RCV;
            }
            else if ((byte == (A_T ^ SETUP) && connectionParameters.role == LlRx) 
                    || (byte == (A_T ^ UA) && connectionParameters.role == LlTx)) {
                *state = BCC_OK;
            }
            else {
                *state = START;
            }
            break;

        case BCC_OK:
            *state = (byte == F) ? STOP : START;
            break;
            
        default:
            break;
    }
}