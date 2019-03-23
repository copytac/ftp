#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/tcp.h>
//#include <poll.h>

#define IP "127.0.0.1"
#define PORT "20021"
#define N 1024

int main(int argc, char const *argv[])
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_sock;
	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = inet_addr(IP);
	server_sock.sin_port = htons(atoi(PORT));
	socklen_t socklen = sizeof(server_sock);

	if(connect(fd, (struct sockaddr *)&server_sock, socklen) < 0)
		exit(-1);
	printf("connect to %s:%d\n", inet_ntoa(server_sock.sin_addr), ntohs(server_sock.sin_port));
	
	//struct timeval timeout={5, 0};
	//setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	//setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
	
	FILE *fp = fdopen(fd, "w+");
	setvbuf(fp, NULL, _IONBF, 0);
	char buf[N] = {0};
	char cmd[4] = {0};

	int status = -1;


	fgets(buf, N, stdin);
	//struct timeval timeout={5, 0};
	//fd_set fds;
	/*struct tcp_info info; 
  	int len = sizeof(info); 
	if(getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len) < 0)
		printf("error tcpinfo\n");
	printf("%d\n", info.tcpi_state);*/
	int optval;
	int optlen = sizeof(int);
	getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&optval, (socklen_t *)&optlen);
			
	while(optval == 0){
		int i = 0;
		while((buf[i] != '\n') && (buf[i] != ' '))
			++i;
		if(i == 0)
			return -1;
		memcpy(cmd, buf, i);

		if(strcmp("ls", cmd) == 0){
			status = 0;
		}
		else if(strcmp("get", cmd) == 0){
			status = 1;
		}
		else{
			fputs("wrong\n", stdout);
			status = -1;
		}
		fputs(buf, fp);

		/*FD_ZERO(&fds);
		FD_SET(fd, &fds);
		int n = select(fd+1, &fds, NULL, NULL, &timeout);
		if(n == -1){
			printf("-1\n");
			break;
		}
		else if(n == 0){
			printf("0\n");
			break;
		}

		else if(FD_ISSET(fd, &fds)){*/
		if(status == 0){
			memset(buf, 0, N);
			//fread(buf, 1, N, fp);
			//fwrite(buf, strlen(buf), 1, stdout);
			fgets(buf, N, fp);
			fputs(buf, stdout);

		}
		else if(status == 1){
			int size = 0;
			read(fd, &size, 4);
			size = ntohl(size);
			printf("%d\n", size);
			if(size == 0){
				fputs("file doesn't exist.", stdout);
				return -1;
			}

			int end = strlen(buf);
			buf[--end] = 0;
			while(buf[--end] != '/')
				;
			FILE *file_fp = fopen(&buf[++end], "w");

			memset(buf, 0, N);
			int i = 0, n = 0;
			for(i = 0; i < size; i+=N){
				n = fread(buf, 1, N, fp);
				fwrite(buf, n, 1, file_fp);
			}
			fclose(file_fp);
		}
		//}
		getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&optval, (socklen_t *)&optlen);
		memset(buf, 0, N);
		fgets(buf, N, stdin);
	}
	close(fd);
	return 0;
}


/*	fd_set rset, allset;
	int maxfd = fd;
	FD_ZERO(&allset);
	FD_SET(fd, &allset);
	select(maxfd+1, &rset, NULL, NULL, NULL);*/
	
	//struct pollfd poll_fd;
	//poll_fd.fd=0;
	//poll_fd.events=POLLIN;
	/*while(fgets(buf, N, fp)){
		fputs(buf, stdout);
		printf("%d\n", ftell(fp));
	}*/