all: ftp client

ftp: ftp.c 
	gcc $^ -o $@ -Wall
client: client.c
	gcc $^ -o $@ -Wall