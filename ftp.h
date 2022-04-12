#ifndef FTP_H
#define FTP_H


int max(int a, int b);
//解析命令
int parse_cmd(char* buf, char* command_argv[], int n);
int get_name_pos_from_namebuf(char namebuf[]) ;

int max(int a, int b) {
    return a > b ? a : b;
}
//解析命令
int parse_cmd(char* buf, char* command_argv[], int n) {
    int i = 0, j = 0;

    for (i = 0; i < n; i++) {
        command_argv[i] = &buf[j];
        while (buf[j] != ' ' && buf[j] != '\n' && buf[j] != '\r')
            j++;
        if (buf[j] == '\r' || buf[j] == '\n') {
            buf[j] = 0;
            break;
        }
        else if (buf[j] == ' ') {
            buf[j++] = 0;
        }
    }
    return 0;
}

int get_name_pos_from_namebuf(char namebuf[]) {
    //buf[end] is 'Enter'.remove it
    int end = strlen(namebuf);
    //buf[end] = 0;
    char c;
    do { c = namebuf[--end]; } while (c != '/' && c != ' ' && end > 0);
    if (end == 0)
        ;
    else
        ++end;

    return end;
}

#endif