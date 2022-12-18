CC = gcc
CFLAGS = -Wall

.PHONY: all
all:
		$(CC) $(CFLAGS)  download.c -o download

.PHONY: clean
clean:
		rm -f download


#///////////////////////////////////////////////////////////
.PHONY: pic1
pic1:
	./download ftp://rcom:rcom@netlab1.fe.up.pt/files/pic1.jpg

.PHONY: pipe
pipe:
	./download ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt