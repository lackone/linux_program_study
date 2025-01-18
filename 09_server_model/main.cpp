#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include "file_server.h"
#include "file_client.h"
#include "udp_room_server.h"
#include "udp_room_client.h"

using namespace std;

//I/O 模型
//阻塞 I/O
//阻塞 I/O 就是当用户进程发起一个 I/O 请求操作（以读取为例），内核将会查看读取的数据是否准备就绪，如果没有准备好，则当前进程被挂起（睡眠），阻塞等待结果返回。
void io_block_test() {
    char buf[1024] = {0};
    while (1) {
        //如果没有用户输入，这里会一直阻塞
        fgets(buf, sizeof(buf), stdin);
        printf("%s\n", buf);
    }
}

//I/O 根据操作对象的不同可以分为内存 I/O、网络 I/O 和磁盘 I/O 三种
//在网络 I/O（如 UDP 编程）中，当用户进程调用 recvfrom()这个系统调用，内核就开始 I/O 的第一阶段：准备数据。
//对于网络 I/O 来说，很多时候数据在一开始并没有到达，这个时候内核就要等待足够的数据到来。
//磁盘 I/O 的情况则是等待磁盘数据从磁盘上读取到内核所访问的内存区域中。 这个过程需要等待，也就是说数据被复制到操作系统内的存储区域是需要一个过程的。
//而在用户进程这边，整个进程会被阻塞。当内核等到数据准备就绪，它就会将数据从内核访问区域中复制到用户进程使用的内存中，之后内核返回结果。 用户进程解除阻塞状态，重新运行。

//非阻塞 I/O
//非阻塞 I/O 与阻塞 I/O 不同的是，当用户进程执行读操作时，如果内核中的数据没有准备就绪，那么它并不会阻塞用户进程，而是立刻返回一个错误码。
//非阻塞 I/O的特点是用户进程需要不断的主动询问内核数据是否准备就绪。

//int fcntl(int fd, int cmd, ... /* arg */ );
//fcntl()函数用来执行对一个已经打开的文件进行操作。
//选用F_GETFL 与 F_SETFL，获取文件的状态标记并设置为非阻塞状态。
void io_noblock_test() {
    //0就是标准输入
    int flag = fcntl(0, F_GETFL);

    //修改其标志位
    flag |= O_NONBLOCK;

    //重新设置修改后的标志位
    fcntl(0, F_SETFL, flag);

    char buf[1024] = {0};
    ssize_t bytes = 0;

    while (1) {
        //不会阻塞，一直读
        bytes = read(0, buf, sizeof(buf));
        printf("%d\n", bytes);

        if (bytes == -1) {
            perror("read");
        } else if (bytes == 0) {
            perror("no data");
        } else {
            buf[bytes] = '\0';
            printf("read %s\n", buf);
        }

        sleep(1);
    }
}

//I/O 多路复用
//I/O 多路复用指的是在一个程序中，跟踪处理多条独立的 I/O 流。
//I/O 多路复用意在处理多个独立的输入/输出操作，其操作对象为文件描述符。
//I/O 多路复用的出现很好地解决了服务器的吞吐能力。
//原因就在于，当不使用 I/O 多路复用时，程序虽然可以处理完所有的 I/O 流，但是如果某个 I/O 流被设置为阻塞的，那么很可能会导致其他 I/O 操作无法执行。

//另一方面，假设所有的 I/O 流被设置为非阻塞，那么在资源并没有准备就绪的情况下，只能采用轮询的机制访问，CPU 将一直被用来判断资源是否准备就绪，这无疑增加了资源的浪费。

//I/O 多路复用的特点是，并不会将 I/O 流（I/O 操作）设置为阻塞或非阻塞。
//而是采用监听的方式监测所有 I/O 操作的对象（文件描述符），检测其是否可以被执行访问。 当某一个文件描述符的 状态为准备就绪时，则可以进行 I/O 操作。

//select()函数的工作原理为阻塞监听所有的文件描述符的状态是否发生变化
//int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
//select()函数做了一个很重要的工作，即将准备就绪的文件描述符留在原有的监听集合中，而集合中没有准备就绪的文件描述符将会被从集合中清除。

//void FD_CLR(int fd, fd_set *set);   FD_CLR()表示将一个给定的文件描述符从集合中删除。
//int FD_ISSET(int fd, fd_set *set);  FD_ISSET()表示判断指定的文件描述符是否在集合中。
//void FD_SET(int fd, fd_set *set);   FD_SET()表示将指定的文件描述符添加到指定的集合中。
//void FD_ZERO(fd_set *set);          FD_ZERO()表示清空整个文件描述符集合。

void select_server_test() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    //1、创建套接字
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    //2、填充服务器网络信息结构体
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    //3、将套接字与服务器网络信息结构体绑定
    bind(server_fd, (sockaddr *) &addr, addr_len);

    //4、将套接字设置为被动监听模式
    listen(server_fd, 5);

    //使用 select()函数实现 I/O 多路复用
    //注意：由于会移除集合里面的文件描述符,所以需要每次都添加
    fd_set read_fds;
    FD_ZERO(&read_fds);

    char buf[1024] = {0};
    int ret;
    int accept_fd;

    while (1) {
        //将需要的文件描述符添加到集合里面
        FD_SET(0, &read_fds);
        FD_SET(server_fd, &read_fds);

        ret = select(server_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
        }

        printf("ret = %d\n", ret);

        //判断是哪个文件描述符准备就绪并执行对应的 I/O 操作
        //由于 select()函数运行结束后,集合里面只剩下准备就绪的
        //因此只要判断到底是哪个文件描述符还在集合里面即可
        if (FD_ISSET(0, &read_fds)) {
            fgets(buf, sizeof(buf), stdin);
            buf[strlen(buf) - 1] = '\0';
            printf("buf = %s\n", buf);
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            if ((accept_fd = accept(server_fd, (sockaddr *) &client, &addr_len)) < 0) {
                perror("accept");
            }

            printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
        }
    }

    close(accept_fd);
    close(server_fd);
}

void select_client_test() {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    connect(server_fd, (sockaddr *) &addr, addr_len);

    close(server_fd);
}

//信号驱动 I/O
//信号驱动 I/O 就是指进程预先告知内核、向内核注册一个信号处理函数，然后用户进程返回不阻塞。
//当内核数据准备就绪时会发送一个信号给进程，用户进程接收信号后，在信号处理函数中调 用 I/O 操作读取数据。

//TCP 服务器模型
//循环服务器
//循环服务器的实现其实很简单，通常可以采用循环嵌套的方式实现。即外层循环依次接收客户端的请求，建立 TCP 连接。
//内层循环接收并处理客户端的所有数据，直到客户端关闭连接。如果当前客户端没有处理结束，其他客户端必须一直等待。
void loop_server_test() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    bind(server_fd, (sockaddr *) &addr, addr_len);

    listen(server_fd, 5);

    int accept_fd;
    int bytes = 0;
    char buf[1024] = {0};

    while (1) {
        accept_fd = accept(server_fd, (sockaddr *) &client, &addr_len);
        printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        while (1) {
            if ((bytes = recv(accept_fd, buf, sizeof(buf), 0)) < 0) {
                perror("recv");
            } else if (bytes == 0) {
                perror("no data");
            } else {
                if (strncmp(buf, "exit", 4) == 0) {
                    printf("client exit\n");
                    break;
                }

                buf[bytes] = '\0';
                printf("client %s\n", buf);
                strcat(buf, "-server");

                if (send(accept_fd, buf, strlen(buf), 0) < 0) {
                    perror("send");
                }
            }
        }
    }
}

void loop_client_test() {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    connect(server_fd, (sockaddr *) &addr, addr_len);

    char buf[1024] = {0};
    int bytes = 0;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        if (send(server_fd, buf, strlen(buf), 0) < 0) {
            perror("send");
        } else {
            if (strncmp(buf, "exit", 4) == 0) {
                break;
            }
            if ((bytes = recv(server_fd, buf, sizeof(buf), 0)) < 0) {
                perror("recv");
            }
            buf[bytes] = '\0';
            printf("server %s\n", buf);
        }
    }
}

//fork()实现并发服务器
//并发服务器解决了循环服务器的缺陷，即可以在同一时刻响应多个客户端的请求。
//其基本设计思想是在服务器端采用多任务 机制（多进程或多线程），分别为每一个客户端创建一个任务处理。 也可以使用 select()函数实现并 发服务器。
void sig_handler(int) {
    //通过信号使父进程执行等待回收子进程的资源的操作
    wait(nullptr);
}

void fork_server_test() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    bind(server_fd, (sockaddr *) &addr, addr_len);

    listen(server_fd, 5);

    //注册信号
    signal(SIGUSR1, sig_handler);

    pid_t pid;
    int accept_fd;
    char buf[1024] = {0};

    while (1) {
        accept_fd = accept(server_fd, (sockaddr *) &client, &addr_len);

        printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        pid = fork();

        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            //子进程专门负责处理客户端的消息，不负责客户端的连接

            //子进程不需要接收客户端的连接请求
            close(server_fd);

            while (1) {
                int bytes;
                if ((bytes = recv(accept_fd, buf, sizeof(buf), 0)) < 0) {
                    perror("recv");
                } else if (bytes == 0) {
                    perror("no data");
                } else {
                    if (strncmp(buf, "exit", 4) == 0) {
                        printf("client exit\n");
                        break;
                    }
                    buf[bytes] = '\0';
                    printf("client %s\n", buf);
                    strcat(buf, "-server");

                    if (send(accept_fd, buf, strlen(buf), 0) < 0) {
                        perror("send");
                    }
                }
            }

            kill(getppid(), SIGUSR1);
            exit(0);
        } else {
            //父进程，释放资源
            close(accept_fd);
        }
    }
}

void fork_client_test() {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    connect(server_fd, (sockaddr *) &addr, addr_len);

    char buf[1024] = {0};
    int bytes = 0;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        if (send(server_fd, buf, strlen(buf), 0) < 0) {
            perror("send");
        } else {
            if (strncmp(buf, "exit", 4) == 0) {
                break;
            }

            if ((bytes = recv(server_fd, buf, sizeof(buf), 0)) < 0) {
                perror("recv");
            }

            buf[bytes] = '\0';
            printf("server %s\n", buf);
        }
    }

    close(server_fd);
}

//select()实现并发服务器
//服务器通过 select()监听用于接收请求的 sockfd，以及用于收发信息的acceptfd，判断其是否准备就绪，从而选择进行接收请求或收发数据。
void select_server_test2() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    bind(server_fd, (sockaddr *) &addr, addr_len);

    listen(server_fd, 5);

    //最大文件描述符
    int accept_fd;
    int max_fd = server_fd;
    int ret;
    int bytes;
    char buf[1024] = {0};
    fd_set read_fds, tmp_fds;
    //清空集合
    FD_ZERO(&read_fds);
    //将需要的文件描述符添加到集合里面
    FD_SET(server_fd, &read_fds);

    while (1) {
        //select会清空read_fds
        tmp_fds = read_fds;

        if ((ret = select(max_fd + 1, &tmp_fds, nullptr, nullptr, nullptr)) < 0) {
            perror("select");
        }

        printf("ret = %d\n", ret);

        //判断是哪个文件描述符准备就绪并执行对应的 I/O 操作
        //由于 select()函数运行结束后,集合里面只剩下准备就绪的
        //因此只要判断到底是哪个文件描述符还在集合里面即可
        for (int i = 0; i < max_fd + 1; i++) {
            if (FD_ISSET(i, &tmp_fds) == 1) {
                if (i == server_fd) {
                    if ((accept_fd = accept(server_fd, (sockaddr *) &client, &addr_len)) < 0) {
                        perror("accept");
                    }

                    printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                    //将需要执行 I/O 操作的文件描述符添加到集合里面
                    FD_SET(accept_fd, &read_fds);
                    printf("add accept_fd %d\n", accept_fd);

                    //需要获取最大的文件描述符
                    max_fd = accept_fd > max_fd ? accept_fd : max_fd;
                } else {
                    if ((bytes = recv(i, buf, sizeof(buf), 0)) < 0) {
                        perror("recv");
                    } else if (bytes == 0) {
                        perror("no data");
                        FD_CLR(i, &read_fds);
                        close(i);
                    } else {
                        if (strncmp(buf, "exit", 4) == 0) {
                            FD_CLR(i, &read_fds);
                            close(i);
                        } else {
                            buf[bytes] = '\0';
                            printf("client %s\n", buf);
                            strcat(buf, "-server");

                            if (send(i, buf, strlen(buf), 0) < 0) {
                                perror("send");
                            }
                        }
                    }
                }
            }
        }
    }
    close(accept_fd);
    close(server_fd);
}

//TCP 文件服务器
//（1）实现客户端查看服务器的目录的所有文件名。
//（2）实现客户端可以下载服务器的目录的文件。
//（3）实现客户端能够上传文件到服务器。

//UDP 网络聊天室
//（1）如果用户登录，其他在线的用户可以收到该用户的登录信息。
//（2）如果用户发送信息，其他在线的用户可以收到该信息。
//（3）如果用户退出，其他在线用户可以收到退出信息。
//（4）同时，服务器可以发送系统信息。

int main(int argc, char *argv[]) {
    //io_block_test();
    //io_noblock_test();
    //if (strcmp(argv[1], "server") == 0) {
    //    select_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    select_client_test();
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    loop_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    loop_client_test();
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    fork_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    fork_client_test();
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    select_server_test2();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    fork_client_test();
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    FileServer fs("127.0.0.1", 6666);
    //    fs.Start();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    FileClient fc("127.0.0.1", 6666);
    //    fc.Start();
    //}
    if (strcmp(argv[1], "server") == 0) {
        UdpRoomServer urs("127.0.0.1", 6666);
        urs.start();
    } else if (strcmp(argv[1], "client") == 0) {
        UdpRoomClient urc("127.0.0.1", 6666);
        urc.start();
    }
    return 0;
}
