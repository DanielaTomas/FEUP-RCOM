// Utils header.

#ifndef _UTILS_H_
#define _UTILS_H_

unsigned char read_message();

int send_message_W(unsigned char* frame_msg, int frame_size,int frame_type);
void send_message(unsigned char CV);
unsigned char read_message();
int state_machine_read(RState *state, unsigned char* reader, int keep_data, unsigned char byte, int frame_type, int size, unsigned char *packet);
int verify_BCC2(unsigned char *packet, int size);
void readControlMessage(unsigned char CV);

#endif // _UTILS_H_
