// Alarm header.

#ifndef _ALARM_H_
#define _ALARM_H_

typedef struct {
  int counter;
  int enabled;
} Alarm;

void alarmConstructor();
void alarmHandler();
int setAlarm(int timeout);

#endif // _ALARM_H_