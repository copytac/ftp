#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netinet/tcp.h>

#define PORT "20021"
#define N 1024

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
/*
int handle(){

}
*/

int main(int argc, char const *argv[])
{
	/* code */
	int server_fd = startup(atoi(PORT));

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

			FILE *fp = fdopen(client_fd, "r+");
			setvbuf(fp, NULL, _IONBF, 0);
			char buf[N] = {0};
			int optval;
			int optlen = sizeof(int);
			getsockopt(client_fd, SOL_SOCKET, SO_ERROR, (char *)&optval, (socklen_t *)&optlen);
			/*struct tcp_info info; 
  			int len = sizeof(info); 
			if(getsockopt(client_fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len) < 0)
				printf("error tcpinfo\n");*/
			while(optval == 0){
				memset(buf, 0, N);
				fgets(buf, N, fp);

				char *command_argv[3] = {0};
				char cmd[4] = {0};
				int i=0;
				while((buf[i] != '\n') && (buf[i] != ' '))
					++i;
				command_argv[0] = cmd;
				memcpy(command_argv[0], buf, i);

				int count = strlen(&buf[++i]);
				buf[i+count-1] = 0;
				command_argv[1] =  &buf[i];
				//memcpy(command_argv[1], &buf[i], count);
				printf("%s", cmd);
				if(strcmp("ls", cmd) == 0){
					if(count == 0){
						char pos[2] = ".";
						command_argv[1] = pos;
					}

					DIR *dfp = opendir(command_argv[1]);
					if(!dfp)
						fputs("dir wrong.\n", fp);
					struct dirent *dp;
					while((dp = readdir(dfp))){
						if(!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
							continue;
						else
							fprintf(fp, "%s ", dp->d_name);
					}
					fputs("\n", fp);
					closedir(dfp);

				}
				else if(strcmp("get", cmd) == 0){
					FILE *file_fp = fopen(command_argv[1], "r");
					if(!file_fp){
						write(client_fd, NULL, 4);
					}
					memset(buf, 0, N);
					struct stat fileinfo;
					fstat(fileno(file_fp), &fileinfo);
					int size = htonl(fileinfo.st_size);
					write(client_fd, &size, 4);

					int i = 0, n = 0;
					for(i = 0; i < fileinfo.st_size; i+=N){
						n = fread(buf, 1, N, file_fp);
						fwrite(buf, n, 1, fp);
					}
					fclose(file_fp);
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