#include <cstring>
#include <signal.h>
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <bits/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
using namespace std;

//字节序
//分为 大端字节序  小端字节序
//大端，整数的高位字节存储在内存的低地址，低位字节存储在内存的高地址处
//小端，整数的高位字存存储在内存的高地址，低位字节存储在内存的低地址处
//PC大多采用小端字节序，小端字节序又称 主机字节序
//大端字节序 又称 网络字节序
void byteorder() {
    union {
        short value;
        char bytes[sizeof(short)];
    } test;
    test.value = 0x0102;
    if (test.bytes[0] == 1 && test.bytes[1] == 2) {
        printf("big endian\n");
    } else if (test.bytes[0] == 2 && test.bytes[1] == 1) {
        printf("little endian\n");
    } else {
        printf("unknown \n");
    }
}

//提供4个函数来完成主机字节序和网络字节序的转换
//unsigned long int htonl();
//unsigned short int htons();
//unsigned long int ntohl();
//unsigned short int ntohs();

//通用socket地址
//struct sockaddr {
//  sa_family_t sa_family;
//  char sa_data[14];
//};

//14字节无法完全容纳多数协议族的地址值，定义了新的结构体
//struct sockaddr_storage {
//  sa_family_t sa_family;
//  unsigned long int __ss_align;
//  char __ss_padding[128-sizeof(__ss_align)];
//};

//unix本地域协议使用如下
//struct sockaddr_un {
//  sa_family_t sin_family;
//  char sun_path[108];
//};

//TCP/IP有sockaddr_in和sockaddr_in6
//struct sockaddr_in {
//  sa_family_t sin_family; //地址族 AF_INET
//  u_int16_t sin_port;     //端口号，要用网络字节序
//  struct in_addr sin_addr;    //IPV4地址结构
//};

//struct in_addr {
//  u_int32_t s_addr; //地址，要用网络字节序
//};

//IP地址转换函数
//in_addr_t inet_addr(const char* str);  //将点分十进制字符串地址转换成网络字节序
//int inet_aton(const char* cp, struct in_addr* inp);
//char* inet_ntoa(struct in_addr in);   //将网络字节序整数转换为字符串地址
void ntoa_test() {
    in_addr addr;
    addr.s_addr = inet_addr("1.2.3.4");
    char *str1 = inet_ntoa(addr);
    printf("%s\n", str1);
    addr.s_addr = inet_addr("10.194.71.60");
    char *str2 = inet_ntoa(addr);
    printf("%s\n", str2);
}

//下面函数完成上面3个函数同样的功能，同时适用于IPV4和IPV6
//int inet_pton(int af, const char* src, void *dst);
//const char* inet_ntop(int af, const void* src, char* dst, socklen_t cnt);

//1、创建SOCKET
//int socket(int domain, int type, int protocol);

//type参数可以使用 SOCK_NONBLOCK 和 SOCK_CLOEXEC
//表示，创建的socket设为非阻塞
//用fork调用创建子进程时，在子进程中关闭该socket

//2、命名socket
//将一个socket与socket地址绑定称为给该socket命名，客户端通常不需要命名socket
//int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen);

//3、监听socket
//int listen(int sockfd, int backlog);
bool stop = false;
//处理 SIGTERM 信号
void handle_term(int sig) {
    stop = true;
}

void socket_test(const char *ip, int port, int backlog) {
    signal(SIGTERM, handle_term);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket failed\n");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("bind failed\n");
        return;
    }

    //完整连接最多有backlog+1个
    if (listen(sock, backlog) == -1) {
        printf("listen failed\n");
        return;
    }

    while (1) {
        sleep(1);
    }

    close(sock);
}

//4、接受连接
//int accept(int sockfd, struct sockaddr* addr, socklen_t *addrlen);

void socket_test2(const char *ip, int port) {
    signal(SIGTERM, handle_term);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket failed\n");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("bind failed\n");
        return;
    }

    //完整连接最多有backlog+1个
    if (listen(sock, 5) == -1) {
        printf("listen failed\n");
        return;
    }

    sockaddr_in client;
    socklen_t client_len = sizeof(client);

    sleep(20); //等待客户端连接和操作

    //accept只是从监听队列中取出连接，不论连接处于何种状态（如ESTABLISHED和CLOSE_WAIT状态），更不关心任何网络状况的变化。
    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);
    if (cfd == -1) {
        printf("accept failed\n");
        return;
    }

    printf("client ip %s : %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
    close(cfd);

    close(sock);
}

//5、发起连接
//int connect(int sockfd, const struct sockaddr* serv_addr, socklen_t addrlen);

//6、关闭连接
//int close(int fd);
//close并非立即关闭连接，而是将引用计数减1，当fd引用计数为0时，才真正关闭连接。
//fork中父进程打开的socket引用计数加1，因此在父子进程中都需要执行close

//如果无论如何都要立即终止连接，可以使用如下的函数
//int shutdown(int sockfd, int howto);

//TCP数据读写
//ssize_t recv(int sockfd, void *buf, size_t len, int flags);
//ssize_t send(int sockfd, const void *buf, size_t len, int flags);

void send_oob(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket failed\n");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("connect failed\n");
        return;
    }

    send(sock, "hello", strlen("hello"), 0);
    //发送的带外数据，只有最后一个字符 t 当成真正的带外数据接收
    send(sock, "test", strlen("test"), MSG_OOB);
    send(sock, "world", strlen("world"), 0);

    close(sock);
}

void recv_oob(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket failed\n");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("bind failed\n");
        return;
    }

    //完整连接最多有backlog+1个
    if (listen(sock, 5) == -1) {
        printf("listen failed\n");
        return;
    }

    sockaddr_in client;
    socklen_t client_len = sizeof(client);

    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);
    if (cfd == -1) {
        printf("accept failed\n");
        return;
    }

    char buf[256] = {0};
    int len;
    len = recv(cfd, buf, sizeof(buf), 0);
    buf[len] = '\0';
    printf("%d %s\n", len, buf);

    len = recv(cfd, buf, sizeof(buf), MSG_OOB);
    buf[len] = '\0';
    printf("%d %s\n", len, buf);

    len = recv(cfd, buf, sizeof(buf), 0);
    buf[len] = '\0';
    printf("%d %s\n", len, buf);
    close(cfd);

    close(sock);
}

//UDP数据读写
//ssize_t recvfrom(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr, socklen_t * addrlen);
//ssize_t sendto(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr, socklen_t addrlen);
//recvfrom/sendto也可以用于面向连接的socket的数据读写，只需把最后2个参数设置为NULL，因为我们已经和对方建立了连接，知道其socket地址了。

//通用数据读写
//ssize_t recvmsg(int sockfd, struct msghdr* msg, int flags);
//ssize_t sendmsg(int sockfd, struct msghdr* msg, int flags);

//struct msghdr {
//  void* msg_name; //socket地址
//  socklen_t msg_namelen;  //socket地址的长度
//  struct iovec* msg_iov;  //分散的内存块
//  int msg_iovlen;         //分散内存块的数量
//  void* msg_control;      //指向辅助数据的起始位置
//  socklen_t msg_controllen;   //辅助数据的大小
//  int msg_flags;          //复制函数中的flags参数，并在调用过程中更新
//};

//struct iovec {
//  void* iov_base;     //内存起始地址
//  size_t iov_len;     //内存的长度
//};

//带外标记
//内核通知应用程序带外数据到达的2种方式：
//1、IO复用产生的异常事件
//2、SIGURG 信号

//判断sockfd是否处于带外标记，返回1，则可以用带MSG_OOB标志的recv来接收带外数据
//int sockatmark(int sockfd);

//地址信息函数
//int getsockname(int sockfd, struct sockaddr* address, socklen_t* address_len); //获取本端socket地址
//int getpeername(int sockfd, struct sockaddr* address, socklen_t* address_len); //获取远端socket地址

//socket选项
//int getsockopt(int sockfd, int level, int option_name, void* option_value, socklen_t* restrict option_len);
//int setsockopt(int sockfd, int level, int option_name, const void* option_value, socklen_t option_len);

//SO_REUSEADDR 选项
//强制使用被处于 TIME_WAIT 状态的连接占用的socket地址
void reuseaddr_test() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    //即使sock处于 TIME_WAIT 状态，与之绑定的socket地址也可以立即被重用
    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(6666);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
}

// SO_RCVBUF 和 SO_SNDBUF
// 表示 TCP 接收缓冲区和发送缓冲区的大小
void buf_change_client(int port, int size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int send_buf = size;
    int len = sizeof(send_buf);
    setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buf, sizeof(send_buf));
    getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &send_buf, (socklen_t *) &len);
    printf("send buf %d\n", send_buf);

    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    char buf[256] = {0};
    memset(buf, 'a', sizeof(buf));
    send(sock, buf, sizeof(buf), 0);

    close(sock);
}

void buf_change_server(int port, int size) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int rcv_buf = size;
    int len = sizeof(rcv_buf);
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buf, sizeof(rcv_buf));
    getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcv_buf, (socklen_t *) &len);
    printf("recv buf %d\n", rcv_buf);

    bind(sock, (struct sockaddr *) &addr, sizeof(addr));

    listen(sock, 5);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);

    char buf[256] = {0};
    while (recv(cfd, buf, sizeof(buf), 0) > 0) {
    }

    close(cfd);

    close(sock);
}

// SO_RCVLOWAT 和 SO_SNDLOWAT
// 表示TCP接收缓冲区和发送缓冲区的低水位标记
// 一般被IO复用系统调用，用来判断socket是否可读或可写

// SO_LINGER
// 用于控制close系统调用在关闭tcp连接时的行为
// struct linger {
//  int l_onoff;  //开启 或 关闭
//  int l_linger; //滞留时间
// };
// l_onoff = 0, 此时 SO_LINGER 不起作用
// l_onoff 不为0，l_linger = 0时，此时close立即返回，丢弃被关闭的socket对应的发送缓冲区中残留的数据
// l_onoff 不为0，l_linger 大于0，发送缓冲区是否有残留数据，socket是否阻塞，如果阻塞，close等待l_linger的时间，发送残留数据，时间内没有发送完，close返回-1并设置errno为EWOULDBLOCK
// 如果非阻塞，close立即返回，需要根据返回值和errno来判断残留数据是否发送完毕。

//网络信息API
// gethostbyname  和  gethostbyaddr
//struct hostent* gethostbyname(const char* name);
//struct hostent* gethostbyaddr(const void* addr, size_t len, int type);

// getservbyname 和 getservbyport
//struct servent* getservbyname(const char* name, const char* proto); //根据名称获取某个服务的完整信息
//struct servent* getservbyport(int port, const char* proto); //根据端口号获取某个服务的完整信息

void get_test(const char *name) {
    hostent *host = gethostbyname(name);

    servent *serv = getservbyname("daytime", "tcp");
    printf("daytime port %d\n", ntohs(serv->s_port));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = serv->s_port;
    addr.sin_addr = *(in_addr *) *host->h_addr_list;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    char buf[256] = {0};
    int len = read(sock, buf, sizeof(buf));
    buf[len] = '\0';
    printf("daytime %s\n", buf);

    close(sock);
}

// getaddrinfo
// 既能通过主机名获取IP地址，也能通过服务名获得端口号
// int getaddrinfo(const char* hostname, const char* service, const struct addrinfo* hints, struct addrinfo** result);
// void freeaddrinfo(struct addrinfo* res);

// getnameinfo
// 通过socket地址同时获得以字符串表示的主机名和服务名
// int getnameinfo(const struct sockaddr* sockaddr, socklen_t addrlen, char* host, socklen_t hostlen, char* serv, socklen_t servlen, int flags);

// 将 getaddrinfo 和 getnameinfo 返回的错误码转换成字符串形式
//const char* gai_strerror(int error);

int main(int argc, char *argv[]) {
    //byteorder();
    //ntoa_test();
    //socket_test(argv[1], atoi(argv[2]), atoi(argv[3]));
    //socket_test2(argv[1], atoi(argv[2]));
    //if (strcmp(argv[1], "send") == 0) {
    //    send_oob(atoi(argv[2]));
    //} else if (strcmp(argv[1], "recv") == 0) {
    //    recv_oob(atoi(argv[2]));
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    buf_change_server(atoi(argv[2]), atoi(argv[3]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    buf_change_client(atoi(argv[2]), atoi(argv[3]));
    //}
    get_test(argv[1]);


    return 0;
}
