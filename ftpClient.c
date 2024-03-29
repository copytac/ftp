#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <netinet/tcp.h>
#include "ftp.h"
//#include <poll.h>

#define IP "127.0.0.1"
#define PORT "20021"
#define N 1024

//char buf[N] = { 0 };
//char cmd[4] = { 0 };

/*void* func_fgets(void* args) {
	fgets(buf, N, stdin);
	return NULL;
}*/

void* func_tcpstate(void* fdp) {
	struct tcp_info info;
	int infolen = sizeof(struct tcp_info);

	int fd = *(int*)fdp;

	fd_set fdset;
	int maxfd = fd > FD_SETSIZE ? fd : FD_SETSIZE;
	struct timeval t;
	t.tv_sec = 1;
	FD_ZERO(&fdset);
	FD_SET(fd, &fdset);
	while (1) {
		select(maxfd, &fdset, NULL, NULL, &t);
		getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&infolen);
		if (info.tcpi_state != TCP_ESTABLISHED) {
			printf("server disconnected.\n");
			exit(0);
		}
	}
}

int main(int argc, char const* argv[]) {
	char* ip = alloca(16);
	int port = 0;

	if (argc == 1) {
		ip = IP;
		port = atoi(PORT);
	}
	else if (argc == 2) {
		ip = IP;
		port = atoi(argv[1]);
	}
	else if (argc == 3) {
		ip = (char*)argv[1];
		port = atoi(argv[2]);
	}
	else
		return -1;

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_sock;
	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = inet_addr(ip);
	server_sock.sin_port = htons(port);
	socklen_t socklen = sizeof(server_sock);

	if (connect(server_fd, (struct sockaddr*)&server_sock, socklen) < 0) {
		printf("connect fall.\n");
		exit(-1);
	}
	else
		printf("connect to %s:%d\n", inet_ntoa(server_sock.sin_addr), ntohs(server_sock.sin_port));

	FILE* server_fp = fdopen(server_fd, "w+");
	setvbuf(server_fp, NULL, _IONBF, 0);

	int status = -1;
	pthread_t thread_tcpstate;
	pthread_create(&thread_tcpstate, NULL, func_tcpstate, &server_fd);

	while (1) {
		char buf[N] = { 0 };
		memset(buf, 0, N);
		printf("\nFTP> ");
		fgets(buf, N, stdin);

		int i = 0;
		while ((buf[i] != '\n') && (buf[i] != ' '))
			++i;
		if (i == 0)
			return -1;
		char cmd[4];
		memset(cmd, 0, sizeof(cmd));
		memcpy(cmd, buf, i);

		if (strcmp("ls", cmd) == 0) {
			status = 0;
		}
		else if (strcmp("get", cmd) == 0) {
			status = 1;
			//stat();
		}
		else if (strcmp("put", cmd) == 0) {
			status = 2;
		}
		else {
			fputs("wrong\n", stdout);
			status = -1;
		}

		fputs(buf, server_fp);

		char* command_argv[4] = { 0 };
		//解析命令和路径
		parse_cmd(buf, command_argv, sizeof(command_argv));
		
		//ls命令
		if (status == 0) {
			memset(buf, 0, N);
			fgets(buf, N, server_fp);
			fputs(buf, stdout);
		}
		//get命令
		else if (status == 1) {
			int size = 0;
			fread(&size, sizeof(size), 1, server_fp);
			size = ntohl(size);
			//printf("%d byte.\n", size);
			if (size == 0) {
				fputs("file doesn't exist.\n", stdout);
				continue;
			}
			else {
				int pos = get_name_pos_from_namebuf(command_argv[1]);

				char* filename = alloca(strlen(&command_argv[1][pos]));
				//printf("%s %d %d\n", filename, pos, (int)strlen(&buf[pos]));
				strcpy(filename, &command_argv[1][pos]);

				FILE* file_fp = NULL;
				int cur = 0;
				//同名文件不存在，直接写
				if (access(filename, F_OK) == -1)
					file_fp = fopen(filename, "wb+");
				//对主机名称和URL查找数据库记录，看看是断点文件还是新文件
				else {
					//若为断点文件，续传
					if (1) {
						file_fp = fopen(filename, "ab+");
						struct stat fileinfo;
						stat(filename, &fileinfo);
						cur = fileinfo.st_size;
					}
					//为新文件，重命名下载
					else {

					}
				}
				//setvbuf(file_fp, NULL, _IONBF, 0);
				//发送断点大小，0为新文件下载
				int temp = htonl(cur);

				fwrite(&temp, sizeof(temp), 1, server_fp);
				fflush(server_fp);
				//接收内容
				int i, n = (size - cur) / N, last = (size - cur) % N;
				for (i = 0; i < n; ++i) {
					memset(buf, 0, sizeof(buf));
					fread(buf, N, 1, server_fp);
					fwrite(buf, N, 1, file_fp);
				}
				memset(buf, 0, sizeof(buf));
				fread(buf, last, 1, server_fp);
				fwrite(buf, last, 1, file_fp);
				fclose(file_fp);
				//printf("%d bytes %s saved.\n", end, &buf[end]);
				printf("file: %s %d bytes, saved %d bytes.\n", filename, size, size - cur);
			}
		}
		//put
		else if (status == 2) {
			struct stat fileinfo;
			int size = 0;
			char* filename = alloca(strlen(command_argv[1]));

			strcpy(filename, command_argv[1]);
			//文件存在，可发
			if (stat(filename, &fileinfo) != -1) {

				FILE* file_fp = fopen(filename, "rb");

				//setvbuf(file_fp, NULL, _IONBF, 0);
				memset(buf, 0, N);
				//发送文件大小，4字节，便于接收端控制读取数目
				size = fileinfo.st_size;
				int temp_buf = htonl(size);
				//printf("%d\n", size);
				fwrite(&temp_buf, sizeof(temp_buf), 1, server_fp);
				//续传文件当前大小
				temp_buf = 0;
				fread(&temp_buf, sizeof(temp_buf), 1, server_fp);

				int cur = ntohl(temp_buf);
				//printf("文件当前位置%d\n", cur);
				//发送内容
				fseek(file_fp, cur, SEEK_SET);

				int i, n = (size - cur) / N, last = (size - cur) % N;
				for (i = 0; i < n; i++) {
					memset(buf, 0, sizeof(buf));
					fread(buf, N, 1, file_fp);
					fwrite(buf, N, 1, server_fp);
				}
				memset(buf, 0, sizeof(buf));
				fread(buf, last, 1, file_fp);
				fwrite(buf, last, 1, server_fp);

				fclose(file_fp);
				printf("file: %s %d bytes, send %d bytes.\n", filename, size, size - cur);

			}
			//直接发大小，0
			else
				fwrite(&size, sizeof(size), 1, server_fp);
		}
	}
	close(server_fd);
	return 0;
}


/*	server_fd_set rset, allset;
	int maxserver_fd = server_fd;
	server_fd_ZERO(&allset);
	server_fd_SET(server_fd, &allset);
	select(maxserver_fd+1, &rset, NULL, NULL, NULL);*/

	//struct pollserver_fd poll_server_fd;
	//poll_server_fd.server_fd=0;
	//poll_server_fd.events=POLLIN;
	/*while(fgets(buf, N, server_fp)){
		fputs(buf, stdout);
		printf("%d\n", ftell(server_fp));
	}*/

	/*server_fd_ZERO(&server_fds);
	server_fd_SET(server_fd, &server_fds);
	int n = select(server_fd+1, &server_fds, NULL, NULL, &timeout);
	if(n == -1){
		printf("-1\n");
		break;
	}
	else if(n == 0){
		printf("0\n");
		break;
	}

	else if(server_fd_ISSET(server_fd, &server_fds)){*/