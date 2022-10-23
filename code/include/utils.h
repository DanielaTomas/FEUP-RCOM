// Utils header.

#ifndef _UTILS_H_
#define _UTILS_H_
/*
  Divide mensagem vinda do ficheiro em packets
*/
unsigned char *parse_message(unsigned char *msg, off_t *index, int *packet_size, off_t file_size);
/*
 Remove header das frames I
*/
unsigned char *remove_header(unsigned char *remove, int size_to_remove, int *size_removed);
/* 
  Verifica se o frame recebido e o final
*/
int msg_ended(unsigned char *packet, unsigned char *end, int packet_len, int end_len);
/*
Altera BCC1 e BCC2
*/
int send_message_W(unsigned char* frame_msg, int frame_size,int frame_type);
/*
  Envia uma frame de controlo
*/
void send_message(int fd, unsigned char CV);
/*
  Verifica se o BCC2 recebido esta correto
*/
int verify_BCC2(unsigned char *packet, int size);
/*
  Adiciona header da Application Layer ao packet
*/
unsigned char *header_app_level(unsigned char *packet, off_t file_size, int *headerSize);
/*
  Cria os pacotes de controlo
*/
unsigned char *control_packetI(unsigned char state, off_t file_size, unsigned char *file_name, int filename_size, int *packetI_size);


#endif // _UTILS_H_
