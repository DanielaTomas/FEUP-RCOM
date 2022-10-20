// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;    

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;


//bytes
#define byte_max 3
// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

// FRAME
#define F 0x7E
#define A_T 0x03
#define A_R 0x01
#define SETUP 0x03
#define UA 0x07
#define BUFFER_SIZE 255
#define OK 0x00
#define END2 0x03

#define ESC 0x7D
#define ESC_F 0x5E
#define ESC_E 0x5D

//Control Values 
#define CV0 0x00
#define CV1 0x40
#define CV2 0x05
#define CV3 0x85
#define CV4 0x01
#define CV5 0x81
#define DISC 0x0B
// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1


// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);
int llopenR(LinkLayer connectionParameters);
int llopenT(LinkLayer connectionParameters);


// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *packet);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(int showStatistics);

typedef enum {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP, BREAK} RState;
int llopenR(LinkLayer connectionParameters);
int llopenT(LinkLayer connectionParameters);

#endif // _LINK_LAYER_H_
