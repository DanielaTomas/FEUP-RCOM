// Alarm header.

#ifndef _ALARM_H_
#define _ALARM_H_

void alarmHandler(int signal);
int setAlarm(int timeout);
int getAlarmCount();
int isAlarmEnabled();

#endif // _ALARM_H_