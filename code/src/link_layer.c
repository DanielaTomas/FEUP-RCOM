#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include "link_layer.h"
#include "state_machine.h"
#include "alarm.h"
#include "utils.h"

#define _POSIX_SOURCE 1 

struct termios oldtio;
struct termios newtio;

unsigned char sflag, buffer[128], *giant_buf = NULL;
unsigned long giant_buf_size = 0;

int fd;
int disc = FALSE;
LinkLayer cp;

int llopen(LinkLayer connectionParameters) {

    cp = connectionParameters;

    // Open serial port device for reading and writing and not as control_valueling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
    fd = open(cp.serialPort, O_RDWR | O_NOCTTY);

    if (fd < 0) {
        perror(cp.serialPort);
        exit(-1);
    }

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1) {
        perror("Error getting new port settings.\n");
        exit(-1);
    }

    // Clear struct for new port settings
    bzero(&newtio, sizeof(newtio));

    newtio.c_cflag = cp.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 1;  // Blocking read until 1 char received

    if(tcflush(fd, TCIOFLUSH) == -1) {
        perror("Error flushing the data.\n");
        exit(-1);
    }

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
        perror("Error setting new port settings.\n");
        exit(-1);
    }
    
    signal(SIGALRM,alarmHandler);
    
    switch(cp.role) {
         case LlRx:
            return llopenR(fd);
            break;
         case LlTx:
            return llopenT(fd,cp.nRetransmissions,cp.timeout);
            break;
         default:
            break;
    } 

    return -1;  
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize) {

    if(giant_buf_size < bufSize*2 + 10) {
        giant_buf = (giant_buf_size == 0) ? malloc(bufSize*2 + 10) : realloc(giant_buf, bufSize*2+10);
    }

    int fsize = header_frame(giant_buf, buf, bufSize, A_T, control_handler(CVDATA, sflag));
    
    int bytes_written = write_cycle(fsize);
    
    int isReceivedP = FALSE;
    int nRetransmissions = 0;
    data = NULL;
    
    alarm_enabled = TRUE;
    alarm(cp.timeout);

    while(isReceivedP == FALSE){

        if(!alarm_enabled) {
            alarm_enabled = TRUE;
            alarm(cp.timeout);

            if(nRetransmissions > 0) {
                printf("Time-out\n");
                return -1;
            }
            else if(cp.nRetransmissions == nRetransmissions){
                printf("Number of retransmissions allowed exceeded\n");
                return -1;
            }
            
            bytes_written += write_cycle(fsize);
            
            nRetransmissions++;
        }

        int bytes_read;
        
        if((bytes_read = read(fd, buffer, MAX_PCK_SIZE)) == -1) {
            printf("Couldn't read (llwrite)\n");
            return -1;
        }
        
        int j = 0;
        do{
            state_machine(buffer[j]);
            if(fstate == END){
                if(address == A_T && (control_value == control_handler(CVRR, FALSE) || control_value == control_handler(CVRR, TRUE))){
                    isReceivedP = TRUE;
                    if(control_value == control_handler(CVRR, sflag)) {
                        printf("Requesting next packet...\n");
                    }
                }
                if(address == A_T && control_value == control_handler(CVREJ, sflag)) {
                    printf("Requesting retransmission...\n");
		            nRetransmissions = 0;
                }
            }
            j++;
        }while(j < bytes_read && isReceivedP == FALSE && alarm_enabled);
        
    }
    
    if(sflag) {
        sflag = FALSE;
    } 
    else {
        sflag = TRUE;
    }
    
    //printf("%d chars written\n",bytes_written);

    return bytes_written;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet) {
    
    if(giant_buf_size < MAX_PCK_SIZE){
        giant_buf = (giant_buf_size == 0)? malloc(MAX_PCK_SIZE): realloc(giant_buf, MAX_PCK_SIZE);
    }

    int isReceivedP = FALSE;
    data = packet;
    int bytes_read;
    
    while(isReceivedP == FALSE){
        if((bytes_read = read(fd, giant_buf, MAX_PCK_SIZE)) == -1) {
            printf("Couldn't read (llread)\n");
            return -1;
        }
        int i = 0;
        do {
            state_machine(giant_buf[i]);

            if(address == A_T) {
                if(fstate == REJECT){
                    create_command_frame(buffer,((control_value == control_handler(CVDATA, FALSE)) ? control_handler(CVREJ, FALSE):control_handler(CVREJ,TRUE)),A_T);
                    write(fd, buffer, 5);
                    printf("REJ sent\n");
                }
                else if(fstate == END) {
                    if(control_value == CVSET){
                        create_command_frame(buffer, CVUA, A_T);
                        write(fd, buffer, 5);
                        printf("UA sent\n");
                    }
                    else if(control_value == control_handler(CVDATA, sflag)){
                        sflag = (sflag)? FALSE : TRUE;
                        create_command_frame(buffer, control_handler(CVRR, sflag), A_T);
                        write(fd, buffer, 5);
                        printf("RR %i sent\n", sflag);
                        return size;
                    }
                    else {
                        create_command_frame(buffer,control_handler(CVRR, sflag), A_T);
                        write(fd, buffer, 5);
                        printf("RR %i sent requesting retransmission\n",sflag);
                    }
                }
            }

            if(control_value == CVDISC) {
                disc = TRUE;
                create_command_frame(buffer, (control_value == control_handler(CVDATA, FALSE) ? control_handler(CVREJ, FALSE):control_handler(CVREJ, TRUE)), A_T);
                write(fd, buffer, 5);
                printf("DISC received\n");
                return -1;
                break;
            }

            i++;

        }while(i < bytes_read && !isReceivedP);
    }
    return -1;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
}
