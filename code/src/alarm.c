#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "../include/link_layer.h"
#include "../include/alarm.h"

Alarm alarm_;

void alarmConstructor() {
    alarm_.counter = 0;
    alarm_.enabled = FALSE;
}

void alarmHandler() {
    alarmConstructor();
    alarm_.counter++;

    printf("Alarm Counter: %d\n", alarm_.counter);
}

int setAlarm(int timeout) {
    (void)signal(SIGALRM, alarmHandler);

    while (alarm_.counter <= timeout) {
        if (alarm_.enabled == FALSE) {
            alarm(timeout);
            alarm_.enabled = TRUE;
        }
    }

    printf("------------------------\n");
    printf("Operation ending\n");
    printf("------------------------\n");

    return 0;
}