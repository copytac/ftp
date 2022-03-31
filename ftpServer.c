#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
//#include <netinet/tcp.h>

#define PORT "20021"
#define N 1024
//启动socket链接
int startup(int port){

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_sock;
	memset(&server_sock, 0, sizeof(server_sock));

	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = htonl(0);
	server_sock.sin_port = htons(port);
	
	socklen_t socklen = sizeof(server_sock);
	bind(server_fd, (struct sockaddr *)&server_sock, socklen);
	
	listen(server_fd, 5);
	return server_fd;
}
//解析命令
int parse_cmd(char *buf, char *command_argv[], int n){
	int i = 0, j = 0;
	for(i = 0; i < n; ++i){
		command_argv[i] = &buf[j];
		while((buf[j] != ' ') && (buf[j] != '\n'))
			++j;
		if(buf[j] == '\n'){
			buf[j] = 0;
			break;
		}
		buf[j] = 0;
		++j;
	}
	return 0;
}

int main(int argc, char const *argv[])
{
	/* code */
	int server_fd = startup(atoi(PORT));
	struct sockaddr_in server_sock;
	socklen_t slen = sizeof(server_sock);
	getsockname(server_fd, (struct sockaddr *)&server_sock, &slen);
	printf("start at IP PORT: %s %d\n", inet_ntoa(server_sock.sin_addr), atoi(PORT));

	struct sockaddr_in client_sock;
	socklen_t socklen = sizeof(client_sock);
	
	int client_fd;
	int n;

	while(1){
		client_fd = accept(server_fd, (struct sockaddr *)&client_sock, &socklen);
		if(client_fd < 0)
			return -1;
		n = fork();
		if(n == 0){
			close(server_fd);
			//struct timeval timeout={5, 0};
			//setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
			//setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
			printf("connect at IP PORT : %s %d\n", inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port));

			FILE *fp = fdopen(client_fd, "r+");
			//非缓冲模式
			setvbuf(fp, NULL, _IONBF, 0);
			char buf[N] = {0};
			int optval;
			int optlen = sizeof(int);
			getsockopt(client_fd, SOL_SOCKET, SO_ERROR, (char *)&optval, (socklen_t *)&optlen);
			/*struct tcp_info info; 
  			int len = sizeof(info); 
			if(getsockopt(client_fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len) < 0)
				printf("error tcpinfo\n");
			printf("%d\n", info.tcpi_state);*/
			while(optval == 0){
				memset(buf, 0, N);
				fgets(buf, N, fp);
				char *command_argv[4] = {0};
				//解析命令和路径
				parse_cmd(buf, command_argv, sizeof(command_argv));
				//ls 处理
				if(strcmp("ls", command_argv[0]) == 0){
					if(command_argv[1] == NULL){
						char pos[2] = ".";
						command_argv[1] = pos;
					}
					DIR *dfp = opendir(command_argv[1]);
					if(dfp){				
						struct dirent *dp;
						//发送全部文件名，一次太多接收buf会装不下，待解决（目前选择舍弃）
						int name_len = 0;
						while((dp = readdir(dfp)) && ((name_len+=dp->d_reclen)) < N){
							if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
								continue;
							else
								fprintf(fp, "%s ", dp->d_name);
						}
						fputs("\n", fp);
						closedir(dfp);
					}
					else
						fprintf(fp, "\"%s\" doesn't exist\n", command_argv[1]);
				}
				//get 处理
				else if(strcmp("get", command_argv[0]) == 0){
					struct stat fileinfo;
					int size = 0;
					//文件存在，可发
					if(stat(command_argv[1], &fileinfo) != -1){

						FILE *file_fp = fopen(command_argv[1], "rb");
						//setvbuf(file_fp, NULL, _IONBF, 0);
						memset(buf, 0, N);
						//发送文件大小，便于接收端控制读取数目
						size = fileinfo.st_size;
						int temp = htonl(size);
						fwrite(&temp, sizeof(temp), 1, fp);

						//续传文件当前大小
						temp = 0;
						fread(&temp, sizeof(temp), 1, fp);
						int cur = ntohl(temp);
						//发送内容
						fseek(file_fp, cur, SEEK_SET);
						int i, n = (size-cur)/N, last = (size-cur)%N;
						for(i = 0; i < n; ++i){
							memset(buf, 0, sizeof(buf));
							fread(buf, N, 1, file_fp);
							fwrite(buf, N, 1, fp);
						}
						memset(buf, 0, sizeof(buf));
						fread(buf, last, 1, file_fp);
						fwrite(buf, last, 1, fp);
						fclose(file_fp);
					}
					//直接发大小，0
					else
						fwrite(&size, sizeof(size), 1, fp);
				}
				getsockopt(client_fd, SOL_SOCKET, SO_ERROR, (char *)&optval, (socklen_t *)&optlen);
			}
			close(client_fd);
		}
		else
			close(client_fd);
	}
	return 0;
}