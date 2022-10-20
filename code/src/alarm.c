#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include "../include/link_layer.h"
#include "../include/alarm.h"

int alarm_enabled = FALSE;
int alarm_count = 0;

void alarmHandler(int signal) {
    alarm_enabled = FALSE;
    alarm_count++;

    printf("Alarm Counter: %d\n", getAlarmCount());
}

int setAlarm(int timeout) {
    (void)signal(SIGALRM, alarmHandler);

    while (getAlarmCount() <= timeout) {
        if (alarm_enabled == FALSE) {
            alarm(timeout);
            alarm_enabled = TRUE;
        }
    }

    printf("------------------------\n");
    printf("Operation ending\n");
    printf("------------------------\n");

    return 0;
}

int getAlarmCount() {
    return alarm_count;
}

int isAlarmEnabled() {
    return alarm_enabled;
}

void setAlarmCount(int ac) {
    alarm_count = ac;
}

void setAlarmEnabled(int ae) {
    alarm_enabled = ae;
}