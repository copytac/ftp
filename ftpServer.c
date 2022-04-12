#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "ftp.h"
//#include <netinet/tcp.h>

#define PORT "20021"
#define N 1024

//启动socket链接
int startup(int port, int n) {

	int server_fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_sock;
	memset(&server_sock, 0, sizeof(server_sock));

	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = htonl(0);
	server_sock.sin_port = htons(port);

	socklen_t socklen = sizeof(server_sock);
	bind(server_fd, (struct sockaddr*)&server_sock, socklen);

	listen(server_fd, n);
	return server_fd;
}

int main(int argc, char const* argv[]) {
	/* code */
	int port, n = 5;
	if (argc == 2)
		port = atoi(argv[1]);
	/*else if (argc == 3) {
		port = atoi(argv[1]);
		n = argv[2];
	}*/
	else
		port = atoi(PORT);
	int server_fd = startup(port, n);

	struct sockaddr_in server_sock;
	socklen_t slen = sizeof(server_sock);
	getsockname(server_fd, (struct sockaddr*)&server_sock, &slen);
	printf("start at IP PORT: %s %d.\n", inet_ntoa(server_sock.sin_addr), port);

	struct sockaddr_in client_sock;
	socklen_t socklen = sizeof(client_sock);

	while (1) {
		fd_set fdset;
		int maxfd = max(server_fd, FD_SETSIZE);
		//struct timeval t;

		FD_ZERO(&fdset);
		FD_SET(server_fd, &fdset);
		int n = select(maxfd, &fdset, NULL, NULL, NULL);
		if (n <= 0) {
			continue;
		}
		if (!FD_ISSET(server_fd, &fdset)) {
			continue;
		}
		else {
			int flags = fcntl(server_fd, F_GETFL, 0);//获取建立的sockfd的当前状态（非阻塞）	
			fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);//将当前sockfd设置为非阻塞

			int client_fd = accept(server_fd, (struct sockaddr*)&client_sock, &socklen);
			if (client_fd < 0)
				return -1;
			int n = fork();
			if (n != 0) {
				close(client_fd);
			}
			else {
				close(server_fd);
				//struct timeval timeout={5, 0};
				//setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
				//setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
				printf("connect at IP PORT : %s %d\n", inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port));
				FILE* client_fp = fdopen(client_fd, "r+");
				//非缓冲模式
				setvbuf(client_fp, NULL, _IONBF, 0);
				char buf[N] = { 0 };

				do {
					fd_set fdset;
					int maxfd = max(client_fd, FD_SETSIZE);
					//struct timeval t;

					FD_ZERO(&fdset);
					FD_SET(client_fd, &fdset);
					int n = select(maxfd, &fdset, NULL, NULL, NULL);
					if (n <= 0) {
						printf("haha\n");
						break;
					}
					if (!FD_ISSET(client_fd, &fdset)) {
						printf("?\n");
						continue;
					}

					memset(buf, 0, N);
					char* p = fgets(buf, N, client_fp);
					if (p == NULL) {
						printf("IP PORT: %s %d closed.\n", inet_ntoa(client_sock.sin_addr), ntohs(client_sock.sin_port));
						break;
					}
					char* command_argv[4] = { 0 };
					//解析命令和路径
					parse_cmd(buf, command_argv, sizeof(command_argv));
					//ls 处理
					if (strcmp("ls", command_argv[0]) == 0) {
						if (command_argv[1] == NULL) {
							char pos[2] = ".";
							command_argv[1] = pos;
						}
						DIR* dfp = opendir(command_argv[1]);
						if (dfp) {
							struct dirent* dp;
							//发送全部文件名，一次太多接收buf会装不下，待解决（目前选择舍弃）
							int name_len = 0;
							while ((dp = readdir(dfp)) && ((name_len += dp->d_reclen)) < N) {
								if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
									continue;
								else
									fprintf(client_fp, "%s ", dp->d_name);
							}
							fputs("\n", client_fp);
							closedir(dfp);
						}
						else
							fprintf(client_fp, "\"%s\" doesn't exist\n", command_argv[1]);
					}
					//get 处理
					else if (strcmp("get", command_argv[0]) == 0) {
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
							fwrite(&temp_buf, sizeof(temp_buf), 1, client_fp);
							//续传文件当前大小
							temp_buf = 0;
							fread(&temp_buf, sizeof(temp_buf), 1, client_fp);
							int cur = ntohl(temp_buf);
							//printf("文件当前位置%d\n", cur);
							//发送内容
							fseek(file_fp, cur, SEEK_SET);
							int i, n = (size - cur) / N, last = (size - cur) % N;
							for (i = 0; i < n; i++) {
								memset(buf, 0, sizeof(buf));
								fread(buf, N, 1, file_fp);
								fwrite(buf, N, 1, client_fp);
							}
							memset(buf, 0, sizeof(buf));
							fread(buf, last, 1, file_fp);
							fwrite(buf, last, 1, client_fp);

							fclose(file_fp);
							printf("file: %s %d bytes, send %d bytes.\n", filename, size, size - cur);

						}
						//直接发大小，0
						else
							fwrite(&size, sizeof(size), 1, client_fp);
					}

					else if (strcmp("put", command_argv[0]) == 0) {
						int size = 0;
						fread(&size, sizeof(size), 1, client_fp);
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

							fwrite(&temp, sizeof(temp), 1, client_fp);
							fflush(client_fp);
							//接收内容
							int i, n = (size - cur) / N, last = (size - cur) % N;
							for (i = 0; i < n; ++i) {
								memset(buf, 0, sizeof(buf));
								fread(buf, N, 1, client_fp);
								fwrite(buf, N, 1, file_fp);
							}
							memset(buf, 0, sizeof(buf));
							fread(buf, last, 1, client_fp);
							fwrite(buf, last, 1, file_fp);
							fclose(file_fp);
							printf("file: %s %d bytes, saved %d bytes.\n", filename, size, size - cur);
						}
					}
				} while (1);
				fclose(client_fp);
				close(client_fd);
			}
		}
	}
	return 0;
}