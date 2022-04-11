all: ftpClient ftpServer

ftpServer: ftpServer.c 
	gcc $^ -o $@ -Wall 
ftpClient: ftpClient.c
	gcc $^ -o $@ -Wall -lpthread