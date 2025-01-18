#ifndef UDP_ROOM_CLIENT_H
#define UDP_ROOM_CLIENT_H

#include "udp_msg.h"

class UdpRoomClient {
public:
    UdpRoomClient(const char *host, int port) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(host);

        server_addr_len = sizeof(server_addr);
    }

    void start() {
        server_fd = socket(AF_INET, SOCK_DGRAM, 0);

        MSG msg;

        msg.type = LOGIN;
        printf("请输入姓名:\n");
        fgets(msg.name, sizeof(msg.name), stdin);
        msg.name[strlen(msg.name) - 1] = '\0';

        sendto(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &server_addr, server_addr_len);

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            //子进程负责发送数据
            while (1) {
                fgets(msg.text, sizeof(msg.text), stdin);
                msg.text[strlen(msg.text) - 1] = '\0';

                if (strncmp(msg.text, "quit", 4) == 0) {
                    msg.type = QUIT;
                    sendto(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &server_addr, server_addr_len);
                    close(server_fd);
                    kill(getppid(), SIGKILL);
                    exit(1);
                }

                msg.type = CHAT;
                sendto(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &server_addr, server_addr_len);
            }
        } else {
            //父进程负责接收数据
            while (1) {
                recvfrom(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &server_addr, &server_addr_len);

                printf("%s\n", msg.text);
            }
        }
    }

    ~UdpRoomClient() {
        close(server_fd);
    }

private:
    int server_fd;
    sockaddr_in server_addr;
    socklen_t server_addr_len;
};

#endif //UDP_ROOM_CLIENT_H
