#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include<arpa/inet.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#define MAX_SIZE 512
#define FTP_PORT 21

int main(int argc, char **argv) {
    
    struct hostent* h;
    char buf[MAX_SIZE], user[50], password[50], host[100], urlPath[MAX_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

    sscanf(argv[1],"ftp://%511s",buf);

    size_t i = strcspn(buf,":"), size = strlen(buf);

    if(i < size) {
        size_t j = strcspn(buf,"@");

        if(j == size) {
            fprintf(stderr,"Usage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
            exit(-1);
        }

        strncpy(user,buf,i);
        user[i] = '\0';
        strncpy(password,buf+i+1,j-i-1);
        password[j-i-1] = '\0';
        i = strcspn(buf+j+1,"/");
        strncpy(host,buf+j+1,i);
        host[i] = '\0';
        strcpy(urlPath,buf+j+i+1);
    } 
    else {
        i = strcspn(buf,"/");
        strncpy(host,buf,i);
        host[i] = '\0';
        strcpy(urlPath,buf+i+1);   
        strcpy(user,"anonymous");
        strcpy(password,"password");
    }

    printf("\nuser: %s\npassword: %s\nhost: %s\nurl-path: %s\n\n",user,password,host,urlPath);

    if ((h = gethostbyname(host)) == NULL) {
        herror("gethostbyname()");
        printf("%s\n",host);
        exit(-1);
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr*) h->h_addr_list[0])));

    //server address handling
    struct sockaddr_in server_addr;
    bzero((char*) &server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*) h->h_addr_list[0]))); //32 bit Internet address network byte ordered
    server_addr.sin_port = htons(FTP_PORT); //server TCP port must be network byte ordered

    //open a TCP socket
    int sockfd;
    if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("socketfd\n");
        exit(-1);
    }

    //connect to the server
    if (connect(sockfd,(struct sockaddr*) &server_addr,sizeof(server_addr)) < 0) {
        printf("%s\n",h->h_addr_list[0]);
        perror("connect\n");
        exit(-1);
    }

    //send a string to the server
    usleep(100000);
    int bytesRead = read(sockfd,buf,MAX_SIZE);
    buf[bytesRead] = '\0';
    printf("%s\n",buf);

    if(strncmp(buf,"220",3) != 0) {
        perror("220\n");
        exit(-1);
    }


    char buf2[519];
    sprintf(buf2,"user %s\r\n",user);
    size_t bytes = write(sockfd, buf2, strlen(buf2));
    bytesRead = read(sockfd,buf,MAX_SIZE);
    buf[bytesRead] = '\0';
    printf("%s\n",buf);

    if(strncmp(buf,"331 Please specify the password.",32) != 0) {
        perror("331 Please specify the password.\n");
        exit(-1);
    }
    

    sprintf(buf2,"pass %s\r\n",password);
    write(sockfd,buf2,strlen(buf2));
    bytesRead = read(sockfd,buf,MAX_SIZE);
    buf[bytesRead] = '\0';
    printf("%s",buf);

    if(strncmp(buf,"230",3) != 0) {
        perror("230\n");
        exit(-1);
    }


    strcpy(buf2,"pasv\r\n");
    write(sockfd,buf2,strlen(buf2));
    bytesRead = read(sockfd,buf,MAX_SIZE);
    buf[bytesRead] = '\0';
    printf("%s",buf);

    if(strncmp(buf,"227",3) != 0) {
        perror("227\n");
        exit(-1);
    }

    int h1 = 0, h2 = 0, h3 = 0, h4 = 0, p1 = 0, p2 = 0, pasvport;
    sscanf(buf,"227 Entering Passive Mode (%i,%i,%i,%i,%i,%i).",&h1,&h2,&h3,&h4,&p1,&p2);
    pasvport = p1*256+p2;


    //server address handling
    struct sockaddr_in server_addrC;
    sprintf(buf,"%i.%i.%i.%i",h1,h2,h3,h4);
    bzero((char*) &server_addrC,sizeof(server_addrC));
    server_addrC.sin_family = AF_INET;
    server_addrC.sin_addr.s_addr = inet_addr(buf); //32 bit Internet address network byte ordered
    server_addrC.sin_port = htons(pasvport); //server TCP port must be network byte ordered

    int sockfdC;
    if ((sockfdC = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socketfdC\n");
        exit(-1);
    }

    //connect to the server
    if (connect(sockfdC,(struct sockaddr*) &server_addrC,sizeof(server_addrC)) < 0) {
        perror("connect\n");
        exit(-1);
    }

    sprintf(buf2,"retr %s\r\n",urlPath);
    bytes = write(sockfd,buf2,strlen(buf2));

    if (bytes > 0) {
        printf("Written %ld bytes\n", bytes);
    }
    else {
        perror("write\n");
        exit(-1);
    }

    bytesRead = read(sockfd,buf,MAX_SIZE);
    buf[bytesRead] ='\0';
    printf("%s\n", buf);

    if(strncmp(buf,"150",3) != 0) {
        perror("150\n");
        exit(-1);
    }

    char* filename = strrchr(urlPath, '/');

    printf("Receiving file %s...\n",filename+1);

    FILE* fd = fopen(filename+1,"w");

    if(!fd) {
        perror("fopen\n");
        exit(-1);
    }

    while((bytesRead = read(sockfdC,buf,MAX_SIZE)) > 0){
        fwrite(buf,1,bytesRead,fd);
        //printf("%s",buf);
    }

    fclose(fd);

    do {
        memset(buf, 0, MAX_SIZE);
        read(sockfd,buf,MAX_SIZE);
    } while (buf[0] < '1' || buf[0] > '5' || buf[3] != ' ');
    printf("%s", buf);
    buf[3] = '\0';

    if(strncmp(buf,"226",3) != 0) {
        perror("226\n");
        exit(-1);
    }

    sprintf(buf2,"QUIT\r\n");
    write(sockfd,buf2,strlen(buf2));
    bytesRead = read(sockfd,buf,MAX_SIZE);
    buf[bytesRead] = '\0';
    printf("%s",buf);

    if(strncmp(buf,"221",3) != 0) {
        perror("221\n");
        exit(-1);
    }

    if (close(sockfd) < 0) {
        perror("close sockfd\n");
        exit(-1);
    }

    if (close(sockfdC) < 0) {
        perror("close sockfdC\n");
        exit(-1);
    }

    return 0;
}