// Alarm header.

#ifndef _ALARM_H_
#define _ALARM_H_



void alarmHandler(int signal);
int setAlarm(int timeout);
int getAlarmCount();
int isAlarmEnabled();
void setAlarmCount(int ac);
void setAlarmEnabled(int ae);

#endif // _ALARM_H_