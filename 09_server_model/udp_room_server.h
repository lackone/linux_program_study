#ifndef UDP_ROOM_SERVER_H
#define UDP_ROOM_SERVER_H

#include <vector>
#include "udp_msg.h"

class UdpRoomServer {
public:
    UdpRoomServer(const char *host, int port) {
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(host);

        server_addr_len = sizeof(server_addr);
    }

    void start() {
        server_fd = socket(AF_INET, SOCK_DGRAM, 0);

        bind(server_fd, (struct sockaddr *) &server_addr, server_addr_len);

        sockaddr_in client_addr;
        MSG msg;
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            return;
        } else if (pid == 0) {
            //子进程
            //子进程负责发送系统信息
            msg.type = CHAT;
            strcpy(msg.name, "server");

            while (1) {
                fgets(msg.text, sizeof(msg.text), stdin);
                msg.text[strlen(msg.text) - 1] = '\0';

                //发送系统信息是通过将子进程当作客户端，将数据发送给服务器，再由服务器转发实现的
                sendto(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &server_addr, server_addr_len);
            }
        } else {
            while (1) {
                recvfrom(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &client_addr, &server_addr_len);

                printf("%d %s %s\n", msg.type, msg.name, msg.text);

                switch (msg.type) {
                    case LOGIN:
                        do_login(msg, client_addr);
                        break;
                    case CHAT:
                        do_chat(msg, client_addr);
                        break;
                    case QUIT:
                        do_quit(msg, client_addr);
                        break;
                }
            }
        }
    }

    void do_login(MSG msg, sockaddr_in client_addr) {
        sprintf(msg.text, "%s login\n", msg.name);

        for (auto &client: clients) {
            sendto(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &client.addr, sizeof(sockaddr_in));
        }

        //将新登录的用户的信息保存在列表里面
        Client client;
        client.addr = client_addr;
        strcpy(client.name, msg.name);

        clients.push_back(client);
    }

    void do_chat(MSG msg, sockaddr_in client_addr) {
        char buf[1024] = {0};
        sprintf(buf, "%s : %s\n", msg.name, msg.text);
        strcpy(msg.text, buf);

        for (auto &client: clients) {
            //自己不接收自己发送的数据
            if (strcmp(msg.name, client.name) == 0) {
                continue;
            }
            sendto(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &client.addr, sizeof(sockaddr_in));
        }
    }

    void do_quit(MSG msg, sockaddr_in client_addr) {
        sprintf(msg.text, "%s quit\n", msg.name);

        //将用户退出的信息发送给其他用户并将其信息从列表删除
        for (auto &client: clients) {
            if (strcmp(msg.name, client.name) == 0) {
                continue;
            }
            sendto(server_fd, &msg, sizeof(msg), 0, (struct sockaddr *) &client.addr, sizeof(sockaddr_in));
        }

        clients.erase(std::remove_if(clients.begin(), clients.end(), [&msg](Client &client) {
            return strcmp(client.name, msg.name);
        }), clients.end());
    }

    ~UdpRoomServer() {
        close(server_fd);
    }

private:
    int server_fd;
    sockaddr_in server_addr;
    socklen_t server_addr_len;
    std::vector<Client> clients;
};

#endif //UDP_ROOM_SERVER_H
