#include <iostream>
#include <cstring>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <sys/uio.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//多种I/O函数
//linux中的send & recv
//ssize_t send(int sockfd, const void *buf, size_t nbytes, int flags);
//ssize_t recv(int sockfd, void *buf, size_t nbytes, int flags);
//MSG_OOB  用于传输带外数据
//MSG_PEEK  验证输入缓冲中是否存在接收的数据
//MSG_DONTWAIT  调用IO函数时不阻塞，用于使用非阻塞IO
//MSG_WAITALL   防止函数返回，直到接收全部请求的字节数

#if defined(__linux__) || defined(__unix__)

void msg_oob_send_test(int port) {
    sockaddr_in recv_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    recv_addr.sin_port = htons(port);

    connect(sock, (struct sockaddr *) &recv_addr, sizeof(recv_addr));

    write(sock, "hello", strlen("hello"));
    send(sock, "test", strlen("test"), MSG_OOB);
    write(sock, "world", strlen("world"));
    send(sock, "ok", strlen("ok"), MSG_OOB);

    close(sock);
}

int clt_sock;

void urg_handler(int sig) {
    char buf[3] = {0};
    int len = recv(clt_sock, buf, sizeof(buf), MSG_OOB);
    buf[len] = '\0';
    printf("urg msg %s\n", buf);
}

void msg_oob_recv_test(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    struct sigaction act;
    act.sa_handler = urg_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

    //修改套接字拥有者
    //ctl_sock引发的SIGURG信号处理进程变为getpid()函数返回的进程
    fcntl(clt_sock, F_SETOWN, getpid());
    sigaction(SIGURG, &act, 0);

    int len;
    char buf[3] = {0};

    while ((len = recv(clt_sock, buf, sizeof(buf), 0)) > 0) {
        if (len == -1) {
            continue;
        }
        buf[len] = '\0';
        printf("%s\n", buf);
    }

    close(clt_sock);
    close(sock);
}

//检查输入缓冲
//MSG_PEEK选项调用recv函数时，即使读取了输入缓冲的数据也不会删除。
//常与MSG_DONTWAIT合作，以非阻塞方式验证待读取数据存在与否
void peek_send(int port) {
    sockaddr_in send_addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    send_addr.sin_family = AF_INET;
    send_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    send_addr.sin_port = htons(port);
    connect(sock, (struct sockaddr *) &send_addr, sizeof(send_addr));

    write(sock, "hello", strlen("hello"));
    close(sock);
}

void peek_recv(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

    int len;
    char buf[256] = {0};

    while (1) {
        len = recv(clt_sock, buf, sizeof(buf), MSG_PEEK | MSG_DONTWAIT);
        if (len > 0) {
            break;
        }
    }
    buf[len] = '\0';
    printf("%s %d\n", buf, len);

    len = recv(clt_sock, buf, sizeof(buf), 0);
    buf[len] = '\0';
    printf("再次读 %s %d\n", buf, len);

    close(clt_sock);
    close(sock);
}

//readv & writev 函数
//对数据进行整合传输及发送的函数
//writev可以将分散保存在多个缓冲中的数据一并发送，readv可以由多个缓冲分别接收
//ssize_t writev(int fieleds, const struct iovec *iov, int iovcnt);
//struct iovec {
//      void * iov_base; //缓冲地址
//      size_t iov_len; //缓冲大小
//};
void writev_test() {
    struct iovec vec[2];
    vec[0].iov_base = (void *) "hello";
    vec[0].iov_len = strlen("hello");
    vec[1].iov_base = (void *) "world";
    vec[1].iov_len = strlen("world");

    //标准输出
    int len = writev(1, vec, 2);
    printf("\n");
    printf("%d\n", len);
}

//ssize_t readv(int filedes, const struct iovec * iov, int iovcnt);
void readv_test() {
    struct iovec vec[2];
    char buf1[256] = {0};
    char buf2[256] = {0};
    vec[0].iov_base = buf1;
    vec[0].iov_len = 5; //长度5
    vec[1].iov_base = buf2;
    vec[1].iov_len = sizeof(buf2);

    //标准输入
    int len = readv(0, vec, 2);
    printf("%d\n", len);
    printf("%s\n", buf1);
    printf("%s\n", buf2);
}

#else

//windows中无法完成针对该可选项的事件处理，通过select函数解决这个问题
//收到out-of-band数据也属于异常，
void windows_msg_oob_recv_test(int port) {
    WSAData wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup failed\n");
        return;
    }
    SOCKADDR_IN srv_addr, clt_addr;
    int clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

    struct timeval tv;
    fd_set read, rtmp, except, etmp;
    FD_ZERO(&read);
    FD_ZERO(&except);
    FD_SET(clt_sock, &read);
    FD_SET(clt_sock, &except);
    int fd;
    int len;
    char buf[256] = {0};

    while (1) {
        rtmp = read;
        etmp = except;

        tv.tv_sec = 5;
        tv.tv_usec = 0;

        fd = select(0, &rtmp, 0, &etmp, &tv);
        if (fd > 0) {
            if (FD_ISSET(clt_sock, &etmp)) {
                len = recv(clt_sock, buf, sizeof(buf), MSG_OOB);
                buf[len] = '\0';
                printf("urg msg %s\n", buf);
            }

            if (FD_ISSET(clt_sock, &rtmp)) {
                len = recv(clt_sock, buf, sizeof(buf), 0);
                if (len == 0) {
                    break;
                } else {
                    buf[len] = '\0';
                    printf("%s\n", buf);
                }
            }
        }
    }

    closesocket(clt_sock);
    closesocket(sock);
    WSACleanup();
}

void windows_msg_oob_send_test(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return;
    }
    SOCKADDR_IN addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    send(sock, "hello", strlen("hello"), 0);
    send(sock, "test", strlen("test"), MSG_OOB);
    send(sock, "world", strlen("world"), 0);
    send(sock, "ok", strlen("ok"), MSG_OOB);

    closesocket(sock);
    WSACleanup();
}

#endif

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "recv") == 0) {
    //    msg_oob_recv_test(atoi(argv[2]));
    //} else if (strcmp(argv[1], "send") == 0) {
    //    msg_oob_send_test(atoi(argv[2]));
    //}
    //if (strcmp(argv[1], "recv") == 0) {
    //    peek_recv(atoi(argv[2]));
    //} else if (strcmp(argv[1], "send") == 0) {
    //    peek_send(atoi(argv[2]));
    //}
    //writev_test();
    //readv_test();
    if (strcmp(argv[1], "recv") == 0) {
        windows_msg_oob_recv_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "send") == 0) {
        windows_msg_oob_send_test(atoi(argv[2]));
    }
    return 0;
}
