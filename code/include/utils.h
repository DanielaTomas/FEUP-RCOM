// Utils header.

#ifndef _UTILS_H_
#define _UTILS_H_

unsigned char *parse_message(unsigned char *msg, off_t *index, int *packet_size, off_t file_size);
unsigned char *remove_header(unsigned char *remove, int size_to_remove, int *size_removed);
int send_message_W(unsigned char* frame_msg, int frame_size,int frame_type);
void send_message(unsigned char CV);
int verify_BCC2(unsigned char *packet, int size);
unsigned char *header_app_level(unsigned char *packet, off_t file_size, int *headerSize);
unsigned char *control_packetI(unsigned char state, off_t file_size, unsigned char *file_name, int filename_size, int *packetI_size);

#endif // _UTILS_H_
