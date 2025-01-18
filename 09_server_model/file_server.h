#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

class FileServer {
public:
    FileServer(const char *host, int port) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(host);

        server_addr_len = sizeof(server_addr);
    }

    void Start() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);

        bind(server_fd, (struct sockaddr *) &server_addr, server_addr_len);

        listen(server_fd, 5);

        int accept_fd;
        sockaddr_in client;
        socklen_t client_len = sizeof(client);
        int bytes = 0;
        char buf[256] = {0};

        while (1) {
            if ((accept_fd = accept(server_fd, (struct sockaddr *) &client, &client_len)) < 0) {
                perror("accept");
            }
            printf("client ip = %s port = %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            while (1) {
                if ((bytes = recv(accept_fd, buf, sizeof(buf), 0)) < 0) {
                    perror("recv");
                } else if (bytes == 0) {
                    printf("client closed\n");
                    break;
                } else {
                    buf[bytes] = '\0';
                    printf("client %s\n", buf);

                    switch (buf[0]) {
                        case 'L':
                            //查看服务器文件
                            file_list(accept_fd);
                            break;
                        case 'G':
                            //下载文件
                            file_get(accept_fd, buf + 2);
                            break;
                        case 'P':
                            //上传文件
                            file_put(accept_fd, buf + 2);
                            break;
                    }
                }
            }

            close(accept_fd);
        }
    }

    void file_list(int fd) {
        DIR *dir = opendir(".");
        struct dirent *dirent;
        char buf[32] = {0};

        if (dir == NULL) {
            perror("opendir");
            return;
        }

        while ((dirent = readdir(dir)) != NULL) {
            if (dirent->d_name[0] == '.') {
                continue;
            }
            strcpy(buf, dirent->d_name);
            strcat(buf, "\r\n");
            send(fd, buf, strlen(buf), 0);
        }

        strcpy(buf, "**OVER**");
        send(fd, buf, strlen(buf), 0);

        printf("文件名发送完毕\n");
    }

    void file_get(int fd, const char *filename) {
        char buf[32] = {0};
        int file = open(filename, O_RDONLY);
        int bytes = 0;

        if (file < 0) {
            //如果文件不存在，则通知客户端
            if (errno == ENOENT) {
                strcpy(buf, "NO_EXIST");
                send(fd, buf, strlen(buf), 0);
                return;
            }
            perror("open error");
            return;
        }

        //如果文件存在，也需要告诉客户端
        //strcpy(buf, "YES_EXIST");
        //send(fd, buf, strlen(buf), 0);

        //读取文件内容并发送
        while ((bytes = read(file, buf, sizeof(buf))) > 0) {
            printf("bytes = %d\n", bytes);
            send(fd, buf, bytes, 0);
        }

        close(file);

        //防止数据粘包
        sleep(1);
        strcpy(buf, "**OVER**");
        send(fd, buf, strlen(buf), 0);

        printf("文件发送完毕\n");
    }

    void file_put(int fd, const char *filename) {
        char buf[32] = {0};
        int bytes = 0;

        int file = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0664);
        if (file < 0) {
            perror("open error");
            return;
        }

        char *ch;
        while ((bytes = recv(fd, buf, sizeof(buf), 0)) > 0) {
            if ((ch = strstr(buf, "**OVER**")) != NULL) {
                write(file, buf, ch - buf);
                break;
            }
            write(file, buf, bytes);
        }

        close(file);
        printf("文件接收完毕\n");
    }

    ~FileServer() {
        close(server_fd);
    }

private:
    int server_fd;
    sockaddr_in server_addr;
    socklen_t server_addr_len;
};

#endif //FILE_SERVER_H
