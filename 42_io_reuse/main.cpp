#include <iostream>
#include <cstring>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//I/O复用
//理解select函数并实现服务器端
//使用select函数可以将多个文件描述符集中到一起统一监视
//1、是否存在套接字接收数据
//2、无需阻塞传输数据的套接字有哪些
//3、哪些套接字发生了异常

//使用select步骤
//1、设置文件描述符，指定监视范围，设置超时
//2、调用select函数
//3、查看调用结果

//FD_ZERO
//FD_SET
//FD_CLR
//FD_ISSET

//int select(int maxfd, fd_set *readset, fd_set *writeset, fd_set *exceptset, const struct timeval *timeout);

#if defined(__linux__) || defined(__unix__)

void select_test() {
    fd_set reads, tmps;

    FD_ZERO(&reads);
    FD_SET(0, &reads);

    struct timeval timeout;
    int len;
    int ret;
    char buf[256] = {0};

    while (1) {
        tmps = reads; //select函数会清空，所以每次重新赋值
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        ret = select(1, &tmps, 0, 0, &timeout);
        if (ret == -1) {
            printf("select error\n");
            break;
        } else if (ret == 0) {
            printf("select timeout\n");
        } else {
            //判断标准输入是否可读
            if (FD_ISSET(0, &tmps)) {
                len = read(0, buf, sizeof(buf));
                buf[len] = '\0';
                printf("msg from console %s\n", buf);
            }
        }
    }
}

//实现IO复用服务器
void io_server_test(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    fd_set reads, tmps;
    struct timeval timeout;
    int max_fd;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    FD_ZERO(&reads);
    FD_SET(sock, &reads);
    max_fd = sock;
    int fd;
    int clt_sock;
    int len;
    char buf[256] = {0};

    while (1) {
        tmps = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if ((fd = select(max_fd + 1, &tmps, 0, 0, &timeout)) == -1) {
            break;
        }
        if (fd == 0) {
            continue;
        }

        for (int i = 0; i < max_fd + 1; i++) {
            if (FD_ISSET(i, &tmps)) {
                if (i == sock) {
                    clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
                    if (clt_sock == -1) {
                        continue;
                    }

                    FD_SET(clt_sock, &reads);
                    max_fd = clt_sock > max_fd ? clt_sock : max_fd;

                    printf("client connect %d\n", clt_sock);
                } else {
                    len = read(i, buf, sizeof(buf));
                    if (len == 0) {
                        FD_CLR(i, &reads);
                        close(i);
                        printf("close client %d\n", i);
                    } else {
                        write(i, buf, len);
                    }
                }
            }
        }
    }
    close(sock);
}

void io_client_test(int port) {
    sockaddr_in addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    int len;
    char buf[256] = {0};

    while (1) {
        fgets(buf, sizeof(buf), stdin);

        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
        }

        write(sock, buf, strlen(buf));
        len = read(sock, buf, sizeof(buf));
        if (len < 0) {
            continue;
        }
        buf[len] = '\0';
        printf("msg from server %s\n", buf);
    }

    close(sock);
}

#else

//windows平台的select函数
//windows平台select函数第一个参数为了保持与unix系统的兼容而添加，并没有特殊意义。
//int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *excepfds, const struct timeval *timeout);
// typedef struct fd_set {
//      u_int fd_count;
//      SOCKET fd_array[FD_SETSIZE];
// };
//windows的fd_set并没有像linux中采用位数组

void windows_io_server_test(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    fd_set reads, tmps;
    struct timeval timeout;
    int max_fd;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    FD_ZERO(&reads);
    FD_SET(sock, &reads);
    max_fd = sock;
    int fd;
    int clt_sock;
    int len;
    char buf[256] = {0};

    while (1) {
        tmps = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if ((fd = select(max_fd + 1, &tmps, 0, 0, &timeout)) == -1) {
            break;
        }
        if (fd == 0) {
            continue;
        }

        for (int i = 0; i < max_fd + 1; i++) {
            if (FD_ISSET(i, &tmps)) {
                if (i == sock) {
                    clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
                    if (clt_sock == -1) {
                        continue;
                    }

                    FD_SET(clt_sock, &reads);
                    max_fd = clt_sock > max_fd ? clt_sock : max_fd;

                    printf("client connect %d\n", clt_sock);
                } else {
                    len = recv(i, buf, sizeof(buf), 0);
                    if (len == 0) {
                        FD_CLR(i, &reads);
                        closesocket(i);
                        printf("close client %d\n", i);
                    } else {
                        send(i, buf, len, 0);
                    }
                }
            }
        }
    }
    closesocket(sock);
    WSACleanup();
}

void windows_io_client_test(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    int len;
    char buf[256] = {0};

    while (1) {
        fgets(buf, sizeof(buf), stdin);

        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
            }

        send(sock, buf, strlen(buf), 0);
        len = recv(sock, buf, sizeof(buf), 0);
        if (len < 0) {
            continue;
        }
        buf[len] = '\0';
        printf("msg from server %s\n", buf);
    }

    closesocket(sock);
    WSACleanup();
}

#endif

int main(int argc, char *argv[]) {
    //select_test();
    //if (strcmp(argv[1], "server") == 0) {
    //    io_server_test(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    io_client_test(atoi(argv[2]));
    //}
    if (strcmp(argv[1], "server") == 0) {
        windows_io_server_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        windows_io_client_test(atoi(argv[2]));
    }
    return 0;
}
