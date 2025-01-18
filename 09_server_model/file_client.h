#ifndef FILE_CLIENT_H
#define FILE_CLIENT_H

class FileClient {
public:
    FileClient(const char *host, int port) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(host);

        server_addr_len = sizeof(server_addr);
    }

    void Start() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);

        connect(server_fd, (struct sockaddr *) &server_addr, server_addr_len);

        char buf[256] = {0};

        while (1) {
            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf) - 1] = '\0';

            if (strncmp(buf, "help", 4) == 0) {
                do_help();
            }
            if (strncmp(buf, "list", 4) == 0) {
                do_list(server_fd);
            }
            if (strncmp(buf, "get", 3) == 0) {
                do_get(server_fd, buf + 4);
            }
            if (strncmp(buf, "put", 3) == 0) {
                do_put(server_fd, buf + 4);
            }
            if (strncmp(buf, "exit", 4) == 0) {
                break;
            }
        }
    }

    void do_help() {
        printf("list    查看服务器所在目录文件名\n");
        printf("get filename    下载文件\n");
        printf("put filename    上传文件\n");
        printf("exit    退出\n");
    }

    void do_list(int fd) {
        char buf[32] = {0};

        strcpy(buf, "L");
        send(fd, buf, strlen(buf), 0);

        //接收文件名并打印
        while (recv(fd, buf, sizeof(buf), 0) > 0) {
            if (strstr(buf, "**OVER**") != NULL) {
                break;
            }
            printf("%s\n", buf);
        }

        printf("文件名接收完毕\n");
    }

    void do_get(int fd, const char *filename) {
        char buf[32] = {0};
        int bytes;

        sprintf(buf, "G %s", filename);
        send(fd, buf, strlen(buf), 0);

        //接收数据，获取文件是否存在的信息
        bytes = recv(fd, buf, sizeof(buf), 0);
        if (strstr(buf, "NO_EXIST") != NULL) {
            printf("文件%s不存在\n", filename);
            return;
        }

        int file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0664);
        if (file < 0) {
            perror("open error");
            return;
        }

        //接收内容并写入文件
        char *ch;
        do {
            printf("bytes = %d\n", bytes);
            if ((ch = strstr(buf, "**OVER**")) != NULL) {
                printf("%d\n", ch - buf);
                write(file, buf, ch - buf);
                break;
            }
            write(file, buf, bytes);
        } while((bytes = recv(fd, buf, sizeof(buf), 0)) > 0);

        close(file);

        printf("文件接收完毕\n");
    }

    void do_put(int fd, const char *filename) {
        char buf[32] = {0};
        int bytes;

        int file = open(filename, O_RDONLY);
        if (file < 0) {
            if (errno == ENOENT) {
                printf("文件%s不存在\n", filename);
                return;
            }
            perror("open error");
            return;
        }

        sprintf(buf, "P %s", filename);
        send(fd, buf, strlen(buf), 0);

        while ((bytes = read(file, buf, sizeof(buf))) > 0) {
            send(fd, buf, bytes, 0);
        }

        sleep(1);
        strcpy(buf, "**OVER**");
        send(fd, buf, strlen(buf), 0);

        close(file);

        printf("文件上传完毕\n");
    }

    ~FileClient() {
        close(server_fd);
    }

private:
    int server_fd;
    sockaddr_in server_addr;
    socklen_t server_addr_len;
};

#endif //FILE_CLIENT_H
