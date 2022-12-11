CC = gcc
CFLAGS = -Wall

.PHONY: all
all:
		$(CC) $(CFLAGS)  download.c -o download

.PHONY: clean
clean:
		rm -f download


#///////////////////////////////////////////////////////////
.PHONY: test
test:
	./download ftp://ftp.up.pt/pub/kodi/timestamp.txt

.PHONY: download
download:
	./download ftp://rcom:rcom@netlab1.fe.up.pt/pipe.txt