#include <cstring>
#include <iostream>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/epoll.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//基于select的IO复用技术速度慢的原因
//1、调用select函数后常见的针对所有文件描述符的循环语句
//2、每次调用select函数时都需要向该函数传递监视对象信息

//实现EPOLL时必要的函数和结构体
//epoll_create  创建保存epoll文件描述符的空间
//epoll_+ctl    向空间注册并注销文件描述符
//epoll_wait    与select函数类似，等待文件描述符发生变化

//epoll中通过如下结构体epoll_event将发生变化的文件描述符单独集中到一起
//struct epoll_event {
//      __uint32_t events;
//      epoll_data_t  data;
//};
//typedef union epoll_data {
//      void *ptr;
//      int fd;
//      __uint32_t u32;
//      __uint64_t u64;
//} epoll_data_t;

//int epoll_create(int size);
//int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
//EPOLL_CTL_ADD     将文件描述符注册到epoll例程
//EPOLL_CTL_DEL     从epoll例程中删除文件描述符
//EPOLL_CTL_MOD     更改注册的文件描述符的关注事件发生情况

//EPOLLIN       需要读取数据
//EPOLLOUT      输出缓冲为空，可以立即发送数据的情况
//EPOLLPRI      收到OOB数据的情况
//EPOLLRDHUP    断开连接或半关闭的情况，在边缘触发方式下非常有用
//EPOLLERR      发生错误的情况
//EPOLLET       以边缘触发的方式得到事件通知
//EPOLLONESHOT  发生一次事件后，相应文件描述符不再收到事件通知

//int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);

void epoll_echo_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int epoll_size = 50;
    int epfd = epoll_create(epoll_size);
    epoll_event *evs = (epoll_event *) malloc(sizeof(epoll_event) * epoll_size);

    //添加sock到epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);

    int clt_sock;
    int cnt;
    int len;
    char buf[256] = {0};

    while (1) {
        cnt = epoll_wait(epfd, evs, epoll_size, -1);
        if (cnt == -1) {
            perror("epoll_wait error");
            continue;
        }

        for (int i = 0; i < cnt; i++) {
            if (evs[i].data.fd == sock) {
                clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
                ev.events = EPOLLIN;
                ev.data.fd = clt_sock;
                //添加新的客户端sock到epoll
                epoll_ctl(epfd, EPOLL_CTL_ADD, clt_sock, &ev);
            } else {
                len = read(evs[i].data.fd, buf, sizeof(buf));
                if (len == 0) {
                    //客户端关闭
                    epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                    close(evs[i].data.fd);
                    printf("client close %d\n", evs[i].data.fd);
                } else {
                    //输出
                    write(evs[i].data.fd, buf, len);
                }
            }
        }
    }

    close(sock);
    close(epfd);
}

void epoll_echo_client(int port) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    char buf[256] = {0};
    int len;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 4) == 0
        ) {
            break;
        }
        write(sock, buf, strlen(buf));
        len = read(sock, buf, sizeof(buf));
        buf[len] = '\0';
        printf("msg from server %s\n", buf);
    }
}

//条件触发 Level Trigger 和 边缘触发 Edge Trigger
//条件触发，只要输入缓冲有数据就会一直通知该事件

//边缘触发，输入缓冲收到数据时仅注册1次该事件，即使输入缓冲中还留有数据，也不会再进行注册。

//epoll默认以条件触发方式工作

void epoll_lt_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int epoll_size = 50;
    int epfd = epoll_create(epoll_size);
    epoll_event *evs = (epoll_event *) malloc(sizeof(epoll_event) * epoll_size);

    //添加sock到epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);

    int clt_sock;
    int cnt;
    int len;
    char buf[2] = {0}; //把缓冲变小

    while (1) {
        cnt = epoll_wait(epfd, evs, epoll_size, -1);
        if (cnt == -1) {
            perror("epoll_wait error");
            continue;
        }
        printf("epoll_wait return \n");
        for (int i = 0; i < cnt; i++) {
            if (evs[i].data.fd == sock) {
                clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
                ev.events = EPOLLIN;
                ev.data.fd = clt_sock;
                //添加新的客户端sock到epoll
                epoll_ctl(epfd, EPOLL_CTL_ADD, clt_sock, &ev);
            } else {
                //会持续触发读，每次读2字节
                len = read(evs[i].data.fd, buf, sizeof(buf));
                if (len == 0) {
                    //客户端关闭
                    epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                    close(evs[i].data.fd);
                    printf("client close %d\n", evs[i].data.fd);
                } else {
                    //输出
                    write(evs[i].data.fd, buf, len);
                }
            }
        }
    }

    close(sock);
    close(epfd);
}

void epoll_et_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int epoll_size = 50;
    int epfd = epoll_create(epoll_size);
    epoll_event *evs = (epoll_event *) malloc(sizeof(epoll_event) * epoll_size);

    //添加sock到epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);

    int clt_sock;
    int cnt;
    int len;
    char buf[2] = {0}; //把缓冲变小

    while (1) {
        cnt = epoll_wait(epfd, evs, epoll_size, -1);
        if (cnt == -1) {
            perror("epoll_wait error");
            continue;
        }
        printf("epoll_wait return \n");
        for (int i = 0; i < cnt; i++) {
            if (evs[i].data.fd == sock) {
                clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
                ev.events = EPOLLIN | EPOLLET; //改变边缘触发
                ev.data.fd = clt_sock;
                //添加新的客户端sock到epoll
                epoll_ctl(epfd, EPOLL_CTL_ADD, clt_sock, &ev);
            } else {
                //会持续触发读，每次读2字节
                len = read(evs[i].data.fd, buf, sizeof(buf));
                if (len == 0) {
                    //客户端关闭
                    epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                    close(evs[i].data.fd);
                    printf("client close %d\n", evs[i].data.fd);
                } else {
                    //输出
                    write(evs[i].data.fd, buf, len);
                }
            }
        }
    }

    close(sock);
    close(epfd);
}

//边缘触发的服务器端实现中必知的两点
//1、通过errno变量验证错误原因
//int errno;  全局变量
// read函数发现输入缓冲中没有数据可读时返回-1，同时在errno中保存EAGAIN常量
//2、为了完成非阻塞（non-blocking）IO，更改套接字特性。
//int fcntl(int filedes, int cmd ...);
// int flag = fcntl(fd, F_GETFL, 0);
// fcntl(fd, F_SETFL, flag|O_NONBLOCK);

//边缘触发中，接收数据时仅注册1次事件，为了判断缓冲中是否还有数据，在read==-1并且errno==EAGAIN时，没有数据可读。
void set_non_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void epoll_et_server2(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int epoll_size = 50;
    int epfd = epoll_create(epoll_size);
    epoll_event *evs = (epoll_event *) malloc(sizeof(epoll_event) * epoll_size);

    //添加sock到epoll
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);

    int clt_sock;
    int cnt;
    int len;
    char buf[2] = {0}; //把缓冲变小

    while (1) {
        cnt = epoll_wait(epfd, evs, epoll_size, -1);
        if (cnt == -1) {
            perror("epoll_wait error");
            continue;
        }
        printf("epoll_wait return \n");
        for (int i = 0; i < cnt; i++) {
            if (evs[i].data.fd == sock) {
                clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

                //设置非阻塞
                set_non_block(clt_sock);

                ev.events = EPOLLIN | EPOLLET; //改变边缘触发
                ev.data.fd = clt_sock;
                //添加新的客户端sock到epoll
                epoll_ctl(epfd, EPOLL_CTL_ADD, clt_sock, &ev);

                printf("client connect %d\n", clt_sock);
            } else {
                while (1) {
                    //会持续触发读，每次读2字节
                    len = read(evs[i].data.fd, buf, sizeof(buf));
                    printf("len = %d\n", len);
                    if (len == 0) {
                        //客户端关闭
                        epoll_ctl(epfd, EPOLL_CTL_DEL, evs[i].data.fd, NULL);
                        close(evs[i].data.fd);
                        printf("client close %d\n", evs[i].data.fd);
                        break;
                    } else if (len < 0) {
                        if (errno == EAGAIN) {
                            //没有数据可读
                            break;
                        }
                    } else {
                        //输出
                        write(evs[i].data.fd, buf, len);
                    }
                }
            }
        }
    }

    close(sock);
    close(epfd);
}

int main(int argc, char *argv[]) {
    if (strcmp(argv[1], "server") == 0) {
        //epoll_echo_server(atoi(argv[2]));
        //epoll_lt_server(atoi(argv[2]));
        //epoll_et_server(atoi(argv[2]));
        epoll_et_server2(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        epoll_echo_client(atoi(argv[2]));
    }
    return 0;
}
