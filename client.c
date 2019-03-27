#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
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
		memset(cmd, 0, sizeof(cmd));
		memcpy(cmd, buf, i);

		if(strcmp("ls", cmd) == 0){
			status = 0;
		}
		else if(strcmp("get", cmd) == 0){
			status = 1;
			//stat();
		}
		else{
			fputs("wrong\n", stdout);
			status = -1;
		}
		fputs(buf, fp);

		if(status == 0){
			memset(buf, 0, N);
			fgets(buf, N, fp);
			fputs(buf, stdout);
		}
		else if(status == 1){
			int size = 0;
			fread(&size, sizeof(size), 1, fp);
			size = ntohl(size);
			printf("%d\n", size);
			if(size != 0){

				int end = strlen(buf);
				buf[--end] = 0;
				while(buf[--end] != '/')
					;
				++end;
				FILE *file_fp = NULL;
				int cur = 0;
				//同名文件不存在，直接写
				if(access(&buf[end], F_OK) == -1)
					file_fp = fopen(&buf[end], "wb+");
				//对主机名称和URL查找数据库记录，看看是断点文件还是新文件
				else{
					//若为断点文件，续传
					if(1){
						file_fp = fopen(&buf[end], "ab+");
						struct stat fileinfo;
						stat(&buf[end], &fileinfo);
						cur = fileinfo.st_size;
					}
					//为新文件，重命名下载
					else{

					}
				}
				//setvbuf(file_fp, NULL, _IONBF, 0);
				//发送断点大小，0为新文件下载
				int temp = htonl(cur);

				fwrite(&temp, sizeof(temp), 1, fp);
				fflush(fp);
				//接收内容
				int i, n = (size-cur)/N, last = (size-cur)%N;
				for(i = 0; i < n; ++i){
					memset(buf, 0, sizeof(buf));
					fread(buf, N, 1, fp);
					fwrite(buf, N, 1, file_fp);
				}
				memset(buf, 0, sizeof(buf));
				fread(buf, last, 1, fp);
				fwrite(buf, last, 1, file_fp);
				fclose(file_fp);
			}
			else
				fputs("file doesn't exist.\n", stdout);
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