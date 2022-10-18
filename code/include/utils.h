// Utils header.

#ifndef _UTILS_H_
#define _UTILS_H_

extern int fd;
unsigned char read_message();

int send_message_W(unsigned char* frame_msg, int frame_size);
int send_message_R(unsigned char CV);

#endif // _UTILS_H_
