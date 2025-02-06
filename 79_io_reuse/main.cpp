#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
using namespace std;

//IO复用使得程序能同时监听多个文件描述符
//IO复用虽然能同时监听多个文件描述符，但它本身是阻塞的，当多个文件描术符同时就绪时，如果不采取额外措施，只能按顺序依次处理每一个文件描术符
//linux下实现IO复用，主要有 select , poll , epoll

//select系统调用
//int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
//nfds一般设置成文件描述符最大值+1

//FD_ZERO(fd_set *fdset);
//FD_SET(int fd, fd_set *fdset);
//FD_CLR(int fd, fd_set *fdset);
//int FD_ISSET(int fd, fd_set *fdset);

//文件描述符就绪条件
//1、socket内核接收缓存区中的字节数大于或等于其低水位标记SO_RCVLOWAT。此时我们可以无阻塞地读该socket，并且读操作返回的字节数大于0。
//2、socket通信的对方关闭连接，此时对该socket的读操作将返回0。
//3、监听socket上有新的连接请求
//4、socket上有未处理的错误。此时我们可以使用getsockopt来读取和清除该错误。

//socket可写：
//1、socket内核发送缓存区中的可用字节数大于或等于其低水位标记SO_SNDLOWAT。此时我们可以无阻塞地写该socket，并且写操作返回的字节数大于0。
//2、socket的写操作被关闭，对写操作被关闭的socket执行写操作将触发一个SIGPIPE信号。
//3、socket使用非阻塞connect连接成功或者失败之后。
//4、socket上有未处理的错误，此时我们可以使用getsockopt来读取和清除该错误。

//网络程序中，socket能处理的异常情况只有一种，socket上接收到带外数据

void select_oob_test(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);

    fd_set read_fs;
    fd_set except_fs;
    FD_ZERO(&read_fs);
    FD_ZERO(&except_fs);

    char buf[256] = {0};

    while (1) {
        memset(buf, 0, sizeof(buf));
        FD_SET(cfd, &read_fs);
        FD_SET(cfd, &except_fs);
        int ret = select(cfd + 1, &read_fs, NULL, &except_fs, NULL);
        if (ret < 0) {
            printf("select error\n");
            break;
        }
        //对于可读事件
        if (FD_ISSET(cfd, &read_fs)) {
            ret = recv(cfd, buf, sizeof(buf) - 1, 0);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes data %s\n", ret, buf);
        } else if (FD_ISSET(cfd, &except_fs)) {
            //对于异常事件，采用带MSG_OOB标志函数读取带外数据
            ret = recv(cfd, buf, sizeof(buf) - 1, MSG_OOB);
            if (ret <= 0) {
                break;
            }
            printf("get %d bytes oob data %s\n", ret, buf);
        }
    }
    close(cfd);
    close(sock);
}

//poll系统调用
//poll调用与select类似，也是在指定时间内轮询一定数量的文件描术符
//int poll(struct pollfd* fds, nfds_t nfds, int timeout);
//nfds监听事件集合fds的大小，timeout超时，单位毫秒，-1时，将阻塞，直到事件发生

//struct pollfd {
//    int fd; //文件描述符
//    short events; //注册事件
//    short revents; //实际发生的事件，由内核填写
//};

void poll_test() {
    pollfd fds[2] = {
        {
            111, POLLIN, 0
        },
        {
            222, POLLIN, 0
        }
    };
    int ret = poll(fds, 2, -1);
    for (int i = 0; i < 2; i++) {
        if (fds[i].revents & POLLIN) {
            int fd = fds[i].fd;
            //处理socket
        }
    }
}

//epoll系统调用
//内核事件表
//int epoll_create(int size);

//操作内核事件表
//int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
//EPOLL_CTL_ADD 注册事件
//EPOLL_CTL_MOD 修改事件
//EPOLL_CTL_DEL 删除事件

//struct epoll_event {
//  __uint32_t events; //事件
//  epoll_data_t data; //用户数据
//};

//int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
void epoll_test() {
    int epfd = epoll_create(1024);
    epoll_event *ev = new epoll_event[1024];
    int ret = epoll_wait(epfd, ev, 1024, -1);
    for (int i = 0; i < ret; i++) {
        int fd = ev[i].data.fd;
        //处理socket
    }
    delete[] ev;
}

//LT 和 ET 模式
//epoll对文件描述符的操作有两种模式，LT（level Trigger 电平触发）模式 和 ET（edge Trigger 边沿触发）
//LT是默认的工作模式，这种模式下，epoll相当于一个效率较高的poll，
//当往epoll内核事件表中注册一个文件描述符上的EPOLLET事件时，epoll将以ET模式来操作该文件描述符，ET模式是epoll的高效模式。
//LT模式下，当epoll_wait有事件发生时，应用程序可以不立即处理，下次调用epoll_wiat时，还会再次向应用程序通告此事件，直到事件被处理。
//ET模式下，当epoll_wait有事件发生时，应用程序必须立即处理，后续epoll_wait不再向应用程序通知这一事件。
//ET模式很大程度上降低了同一个事件被重复触发的次数
int setnonblocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void addfd(int epfd, int fd, bool enable_et) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et) {
        ev.events |= EPOLLET;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

void epoll_lt_test(epoll_event *events, int num, int epfd, int listenfd) {
    char buf[10] = {0};
    for (int i = 0; i < num; i++) {
        int fd = events[i].data.fd;
        if (fd == listenfd) {
            //处理连接
            sockaddr_in client;
            socklen_t client_len = sizeof(client);
            int cfd = accept(listenfd, (struct sockaddr *) &client, &client_len);
            addfd(epfd, cfd, false);
        } else if (events[i].events & EPOLLIN) {
            //只要读缓存中还有数据未读出，会一直触发
            printf("event trigger \n");
            memset(buf, 0, sizeof(buf));
            int len = recv(fd, buf, sizeof(buf) - 1, 0);
            if (len <= 0) {
                close(fd);
                continue;
            }
            printf("get %d bytes data %s\n", len, buf);
        } else {
            printf("something else happened \n");
        }
    }
}

void epoll_et_test(epoll_event *events, int num, int epfd, int listenfd) {
    char buf[10] = {0};
    for (int i = 0; i < num; i++) {
        int fd = events[i].data.fd;
        if (fd == listenfd) {
            //处理连接
            sockaddr_in client;
            socklen_t client_len = sizeof(client);
            int cfd = accept(listenfd, (struct sockaddr *) &client, &client_len);
            addfd(epfd, cfd, true); //开启ET模式
        } else if (events[i].events & EPOLLIN) {
            //这段代码不会重复触发
            printf("event trigger \n");
            while (1) {
                memset(buf, 0, sizeof(buf));
                int len = recv(fd, buf, sizeof(buf) - 1, 0);
                if (len < 0) {
                    //非阻塞模式，表示数据已经读完，break，等待下一次触发
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        printf("read later\n");
                        break;
                    }
                    close(fd);
                    break;
                } else if (len == 0) {
                    close(fd);
                } else {
                    printf("get %d bytes data %s\n", len, buf);
                }
            }
        }
    }
}

void epoll_server_test(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    epoll_event evs[1024];
    int epfd = epoll_create(1024);
    addfd(epfd, sock, true);

    while (1) {
        int ret = epoll_wait(epfd, evs, 1024, -1);
        if (ret < 0) {
            printf("epoll_wait error\n");
            break;
        }
        //epoll_lt_test(evs, ret, epfd, sock); //lt模式
        epoll_et_test(evs, ret, epfd, sock); //et模式
    }

    close(sock);
}

//EPOLLONESHOT事件
//即使使用ET模式，一个socket上的某个事件还是可能被触发多次，在并发程序中就会引起问题
//比如一个线程在读取完某个socket上的数据后开始处理这些数据，而在数据处理过程中，该socket上又有新数据可读，此时另外一个线程被唤醒来读取这些新数据，于是就出现了2个线程同时操作一个socket的局面
//我们期望一个socket连接任一时刻都只被一个线程处理

//注册了EPOLLONESHOT事件的文件描述符，最多触发其上注册的一个可读，可写或者异常事件，且只触发一次。
void addfd2(int epfd, int fd, bool oneshot) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    if (oneshot) {
        ev.events |= EPOLLONESHOT;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

//重置fd事件，操作系统仍然会触发fd上的EPOLLIN事件，且只触发一次
void reset_oneshot(int epfd, int fd) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
}

struct fds {
    int epfd;
    int sockfd;
};

//工作线程
void *worker(void *arg) {
    int sockfd = ((fds *) arg)->sockfd;
    int epfd = ((fds *) arg)->epfd;

    printf("start new thread to receive data on sockfd %d\n", sockfd);

    char buf[1024] = {0};

    while (1) {
        int len = recv(sockfd, buf, sizeof(buf) - 1, 0);
        if (len == 0) {
            close(sockfd);
            printf("close the connection\n");
            break;
        } else if (len < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                reset_oneshot(epfd, sockfd);
                printf("read later\n");
                break;
            }
        } else {
            printf("get data %s\n", buf);

            sleep(5); //睡眠，模拟数据处理
        }
    }

    printf("end thread receiving data on sockfd %d\n", sockfd);

    return NULL;
}

void epolloneshot_test(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    epoll_event evs[1024];
    int epfd = epoll_create(1024);
    addfd2(epfd, sock, false); //这里不能用oneshot

    while (1) {
        int ret = epoll_wait(epfd, evs, 1024, -1);
        if (ret < 0) {
            printf("epoll_wait error\n");
            break;
        }
        for (int i = 0; i < ret; i++) {
            int fd = evs[i].data.fd;
            if (fd == sock) {
                sockaddr_in client;
                socklen_t client_len = sizeof(client);
                int cfd = accept(sock, (struct sockaddr *) &client, &client_len);
                addfd2(epfd, cfd, true);
            } else if (evs[i].events & EPOLLIN) {
                pthread_t thread;
                fds fds_thread;
                fds_thread.epfd = epfd;
                fds_thread.sockfd = fd;
                //创建一个线程为sock服务
                pthread_create(&thread, NULL, worker, &fds_thread);
            } else {
                printf("something else happened\n");
            }
        }
    }

    close(sock);
}

//非阻塞connect
int unblock_connect(const char *ip, int port, int time) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    int oldopt = setnonblocking(sock);

    int ret = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (ret == 0) {
        //连接成功，恢复属性，并返回
        printf("connect success\n");
        fcntl(sock, F_SETFL, oldopt);
        return sock;
    } else if (errno != EINPROGRESS) {
        printf("unblock connect not support\n");
        return -1;
    }

    //当你尝试执行一个非阻塞性的操作（比如连接到一个服务器），而这个操作不能立即完成时，操作系统会返回 "EINPROGRESS" 错误码。
    //调用select, poll函数来监听这个连接失败的socket上的可写事件
    //当select，poll函数返回后，利用getsockopt来读取错误码并清除该socket上的错误，如果错误码为0，表示连接成功，否则失败。

    fd_set read_fd;
    fd_set write_fd;
    struct timeval tv;

    FD_ZERO(&read_fd);
    FD_ZERO(&write_fd);

    tv.tv_sec = time;
    tv.tv_usec = 0;

    ret = select(sock + 1, NULL, &write_fd, NULL, &tv);
    if (ret < 0) {
        printf("connection timeout\n");
        close(sock);
        return -1;
    }
    if (!FD_ISSET(sock, &write_fd)) {
        printf("no events on sock found\n");
        close(sock);
        return -1;
    }
    int error = 0;
    socklen_t length = sizeof(error);
    //调用getsockopt来获取并清除sock上的错误
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &length) < 0) {
        printf("getsockopt error\n");
        close(sock);
        return -1;
    }
    if (error != 0) {
        printf("connection failed after select with the error %d\n", error);
        close(sock);
        return -1;
    }

    //连接成功
    printf("connection ready after select with the sock %d\n", sock);
    fcntl(sock, F_SETFL, oldopt);
    return sock;
}

//聊天室程序
//
struct client_data {
    sockaddr_in addr;
    char *write_buf;
    char buf[256];
};

void chat_room_server(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    //申请足够大的用户数据，保证socket的值直接用来索引
    client_data *users = new client_data[65535];
    //限制5个
    pollfd fds[6];

    for (int i = 0; i < 6; i++) {
        fds[i].fd = -1;
        fds[i].events = 0;
    }

    fds[0].fd = sock;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    int user_cnt = 0;

    while (1) {
        int ret = poll(fds, user_cnt + 1, -1);
        if (ret < 0) {
            printf("poll error\n");
            break;
        }
        for (int i = 0; i < user_cnt + 1; i++) {
            if ((fds[i].fd == sock) && (fds[i].revents & POLLIN)) {
                //有新连接进来
                sockaddr_in client;
                socklen_t client_len = sizeof(client);
                int cfd = accept(sock, (struct sockaddr *) &client, &client_len);
                if (cfd < 0) {
                    printf("errno %d\n", errno);
                    continue;
                }
                if (user_cnt >= 5) {
                    send(sock, "too many users\n", strlen("too many users\n"), 0);
                    close(cfd);
                    continue;
                }
                //对于新的连接
                user_cnt++;
                //用socket值做索引，保存用户信息
                users[cfd].addr = client;
                setnonblocking(cfd);
                fds[user_cnt].fd = cfd;
                fds[user_cnt].events = POLLIN | POLLRDHUP | POLLERR;
                fds[user_cnt].revents = 0;

                printf("a new user now have %d users\n", user_cnt);
            } else if (fds[i].revents & POLLERR) {
                //如果出现错误
                printf("get an error from %d\n", fds[i].fd);
                char errors[100] = {0};
                socklen_t length = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0) {
                    printf("get socket option failed\n");
                }
                continue;
            } else if (fds[i].revents & POLLRDHUP) {
                //客户端退出，用最后一个客户的数据覆盖退出的客户，然后减user_cnt
                users[fds[i].fd] = users[fds[user_cnt].fd];
                close(fds[i].fd);
                fds[i] = fds[user_cnt];
                i--;
                user_cnt--;
                printf("a client left\n");
            } else if (fds[i].revents & POLLIN) {
                //有数据可读时
                int fd = fds[i].fd;
                memset(users[fd].buf, 0, sizeof(users[fd].buf));
                int len = recv(fd, users[fd].buf, sizeof(users[fd].buf) - 1, 0);
                printf("get %d bytes data %s from %d\n", len, users[fd].buf, fd);
                if (len < 0) {
                    //如果读出错，则关闭连接
                    if (errno != EAGAIN) {
                        close(fd);
                        users[fds[i].fd] = users[fds[user_cnt].fd];
                        fds[i] = fds[user_cnt];
                        i--;
                        user_cnt--;
                    }
                } else if (len == 0) {
                } else {
                    //如果收到数据，则通知其他socket准备写数据
                    for (int j = 1; j <= user_cnt; j++) {
                        if (fds[j].fd == fd) {
                            continue;
                        }
                        printf("sock %d\n", fds[j].fd);
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[fd].buf;
                    }
                }
            } else if (fds[i].revents & POLLOUT) {
                int fd = fds[i].fd;
                if (!users[fd].write_buf) {
                    continue;
                }
                send(fd, users[fd].write_buf, strlen(users[fd].write_buf), 0);
                users[fd].write_buf = NULL;
                //写完数据后，重新注册可读事件
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete[] users;
    close(sock);
}

void chat_room_client(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("connect failed\n");
        close(sock);
        return;
    }

    pollfd fds[2];
    fds[0].fd = 0; //标准输入
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sock;
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    int pipefd[2];
    pipe(pipefd);
    char buf[1024] = {0};

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            printf("poll error\n");
            break;
        }
        if (fds[1].revents & POLLRDHUP) {
            //远端关闭
            printf("server close the connection\n");
            break;
        }
        if (fds[1].revents & POLLIN) {
            //有数据可读
            memset(buf, 0, sizeof(buf));
            recv(fds[1].fd, buf, sizeof(buf) - 1, 0);
            printf("%s\n", buf);
        }
        if (fds[0].revents & POLLIN) {
            printf("数据输入\n");
            //标准输入有数据
            //把标准输入的数据重定向到sock
            splice(0, NULL, pipefd[1], NULL, 65535, SPLICE_F_MORE | SPLICE_F_MOVE);
            splice(pipefd[0], NULL, sock, NULL, 65535, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }

    close(sock);
}

//同时处理TCP和UDP服务
//
void addfd3(int epfd, int fd) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

void handle_tcp_udp(int port) {
    //创建tcp socket，并绑定端口
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(tcp_sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(tcp_sock, 5);

    //创建udp socket，并绑定端口
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(udp_sock, (struct sockaddr *) &addr, sizeof(addr));

    epoll_event evs[1024];
    int epfd = epoll_create(1024);
    addfd3(epfd, tcp_sock);
    addfd3(epfd, udp_sock);

    while (1) {
        int ret = epoll_wait(epfd, evs, 1024, -1);
        if (ret < 0) {
            printf("epoll_wait error\n");
            break;
        }
        for (int i = 0; i < ret; i++) {
            int fd = evs[i].data.fd;
            if (fd == tcp_sock) {
                sockaddr_in client;
                socklen_t client_len = sizeof(client);
                int cfd = accept(tcp_sock, (struct sockaddr *) &client, &client_len);
                addfd3(epfd, cfd);
            } else if (fd == udp_sock) {
                char buf[1024] = {0};
                sockaddr_in client;
                socklen_t client_len = sizeof(client);
                ret = recvfrom(fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *) &client, &client_len);
                if (ret > 0) {
                    sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &client, client_len);
                }
            } else if (evs[i].events & EPOLLIN) {
                char buf[1024] = {0};
                while (1) {
                    memset(buf, 0, sizeof(buf));
                    ret = recv(fd, buf, sizeof(buf) - 1, 0);
                    if (ret < 0) {
                        if ((ret == EAGAIN) || (ret == EWOULDBLOCK)) {
                            break;
                        }
                        close(fd);
                        break;
                    } else if (ret == 0) {
                        close(fd);
                    } else {
                        send(fd, buf, ret, 0);
                    }
                }
            }
        }
    }

    close(tcp_sock);
    close(udp_sock);
}


int main(int argc, char *argv[]) {
    //select_oob_test(atoi(argv[1]));
    //epoll_server_test(atoi(argv[1]));
    //epolloneshot_test(atoi(argv[1]));
    //unblock_connect(argv[1], atoi(argv[2]), 10);
    //if (strcmp(argv[1], "server") == 0) {
    //    chat_room_server(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    chat_room_client(atoi(argv[2]));
    //}
    handle_tcp_udp(atoi(argv[1]));
    return 0;
}
