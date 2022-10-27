
#include "utils.h"
#include "state_machine.h"
#include "link_layer.h"
#include "alarm.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>




int llopenR(int fd) {
    fstate = START;
    alarm_count = 0;
    int set = FALSE, byte;

    while(set == FALSE) {
            
        if((byte = read(fd,buf,MAX_PACKET_SIZE)) == -1) {
            perror("Couldn't read (llopen)\n");
            exit(-1);
        }
        int i = 0;
        do {
            state_machine(buf[i]);
            if(fstate == END && address == A_T) {
                if(control_value == CVSET) {
                    set = TRUE;
                }
                else if(control_value == CVDISC) {
                    disc = TRUE;
                    printf("DISC Received\n");
                    return -1;
                }
            }
            i++;
        }while(i < byte && set == FALSE);
    }

    if(set == TRUE) {
        printf("Set Received\n");
    }

    int size = buildCommandFrame(buf,A_T,CVUA);
    
    write(fd,buf,size); //sends UA reply.

    printf("UA Sent\n");

    return 1;
}

int llopenT(int fd, int nRetransmissions, int timeout) {
        fstate = START;
        alarm_count = 0;
        int ua = FALSE, byte;
        
        while(ua == FALSE && alarm_count < nRetransmissions) {
            alarm(timeout);
            alarm_enabled = TRUE;

            if(alarm_count > 0) {
                printf("Time-out\n");
            }

            int size = buildCommandFrame(buf,A_T,CVSET);
            
            printf("SET sent\n");
            write(fd,buf,size);
            
            while(alarm_enabled && ua == FALSE){

                if((byte = read(fd,buf,MAX_PACKET_SIZE)) == -1) {
                    printf("Couldn't read (llopen)\n");
                    return -1;
                }

                int i = 0;
                
                do {
                    state_machine(buf[i]);
                    if(fstate == END && address == A_T && control_value == CVUA) {
                        ua = TRUE;
                    }
                    i++;
                }while(i < byte && ua == FALSE);
            }
        }
        
        if(ua == TRUE) {
            printf("UA Received\n");
            return 1;
            
        }
        return -1;
}


