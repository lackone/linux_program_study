#include <iostream>
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

//套接字的可选项和I/O缓冲大小
// getsockopt & setsockopt
// int getsockopt(int sock, int level, int optname, void *optval, socklen_t *optlen);
// int setsockopt(int sock, int level, int optname, const void *optval, socklen_t optlen);

//SOL_SOCKET    SO_TYPE     套接字类型
//SOL_SOCKET    SO_SNDBUF SO_RCVBUF     缓冲区大小
//SOL_SOCKET    SO_REUSEADDR    重新使用地址
//IPPROTO_TCP   TCP_NODELAY    是否禁用Nagle算法

#if defined(__linux__) || defined(__unix__)

void sockopt_test() {
    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    int sock2 = socket(AF_INET, SOCK_DGRAM, 0);

    printf("sock1: %d\n", SOCK_STREAM);
    printf("sock2: %d\n", SOCK_DGRAM);

    int sock_type;
    socklen_t sock_type_len = sizeof(sock_type);
    //获取sock的类型
    getsockopt(sock1, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &sock_type_len);
    printf("sock1 sock_type = %d\n", sock_type);

    getsockopt(sock2, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &sock_type_len);
    printf("sock2 sock_type = %d\n", sock_type);

    int buf_size;
    socklen_t buf_size_len = sizeof(buf_size);
    getsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (void *) &buf_size, &buf_size_len);
    printf("send buf size = %d\n", buf_size);

    getsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (void *) &buf_size, &buf_size_len);
    printf("recv buf size = %d\n", buf_size);

    //修改sock
    int snd_size = 1024 * 3;
    socklen_t snd_size_len = sizeof(snd_size);
    int rcv_size = 1024 * 3;
    socklen_t rcv_size_len = sizeof(rcv_size);
    if (setsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (void *) &snd_size, snd_size_len) == -1) {
        perror("setsockopt");
    }
    if (setsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (void *) &rcv_size, rcv_size_len) == -1) {
        perror("setsockopt");
    }

    getsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (void *) &buf_size, &buf_size_len);
    printf("send buf size = %d\n", buf_size);

    getsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (void *) &buf_size, &buf_size_len);
    printf("recv buf size = %d\n", buf_size);
}

//发生地址分配错误 (binding error)
//客户端输入Q或者Ctrl+C,会向服务器传递FIN消息，强制终止程序时，操作系统关闭文件及套接字，相当于调用close，也会传递FIN消息。这通常没问题。
//但如果是服务器突然终止程序，服务器重新运行时，如果用同一端口，将输出 bind error 消息，大约3分钟即可重新运行服务端。

//Time-wait状态
//只有先断开连接的套接字会经过Time-wait过程
//如果服务器先断开，则套接字处于Time-wait，无法立即重新运行，相应端口是正在使用的状态。
//为什么不需要考虑客户端的Time-wait状态，因为客户端套接字的端口号是任意指定的。

//地址再分配
//系统发生故障从而紧急停止，这时需要快速重启服务器以提供服务，Time-wait状态必须等待几分钟。

void echo_server_test(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    //可将Time-wait状态下的套接字端口号重新分配给新的套接字
    int option = 1;
    socklen_t option_len = sizeof(option);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &option, option_len);

    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        printf("bind error\n");
        return;
    }

    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
    if (clt_sock == -1) {
        printf("accept error\n");
        return;
    }

    int len;
    char msg[256] = {0};

    while ((len = read(clt_sock, msg, sizeof(msg))) > 0) {
        write(clt_sock, msg, len);
        write(1, msg, len);
    }

    close(clt_sock);
    close(sock);
}

//TCP_NODELAY
//Nagle算法
//防止因数据包过多而发生网络过载，Nagle算法在1984年诞生，应用于TCP层。
//只有收到前一数据的ACK消息时，Nagle算法才发送下一数据。
//典型的是“传输大文件数据”，将文件数据传入输出缓冲不会花太多时间，不使用Nagle算法，也会在装满输出缓冲时传输数据包。
//这不仅不会增加数据包的数量，反而会在无需等待ACK的前提下连续传输，大大提高传输速度。

//禁用Nagle算法
//int opt_val = 1;
//setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&opt_val, sizeof(opt_val));

#else

//基于windows的实现
//int getsockopt(SOCKET sock, int level, int optname, char *optval, int *optlen);
//int setsockopt(SOCKET sock, int level, int optname, const char *optval, int optlen);

void windows_sockopt_test() {
    WSAData wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        printf("WSAStartup error\n");
        return;
    }

    int sock1 = socket(AF_INET, SOCK_STREAM, 0);
    int sock2 = socket(AF_INET, SOCK_DGRAM, 0);

    printf("sock1: %d\n", SOCK_STREAM);
    printf("sock2: %d\n", SOCK_DGRAM);

    int sock_type;
    socklen_t sock_type_len = sizeof(sock_type);
    //获取sock的类型
    getsockopt(sock1, SOL_SOCKET, SO_TYPE, (char *) &sock_type, &sock_type_len);
    printf("sock1 sock_type = %d\n", sock_type);

    getsockopt(sock2, SOL_SOCKET, SO_TYPE, (char *) &sock_type, &sock_type_len);
    printf("sock2 sock_type = %d\n", sock_type);

    int buf_size;
    socklen_t buf_size_len = sizeof(buf_size);
    getsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (char *) &buf_size, &buf_size_len);
    printf("send buf size = %d\n", buf_size);

    getsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (char *) &buf_size, &buf_size_len);
    printf("recv buf size = %d\n", buf_size);

    //修改sock
    int snd_size = 1024 * 3;
    socklen_t snd_size_len = sizeof(snd_size);
    int rcv_size = 1024 * 3;
    socklen_t rcv_size_len = sizeof(rcv_size);
    if (setsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (char *) &snd_size, snd_size_len) == -1) {
        perror("setsockopt");
    }
    if (setsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (char *) &rcv_size, rcv_size_len) == -1) {
        perror("setsockopt");
    }

    getsockopt(sock1, SOL_SOCKET, SO_SNDBUF, (char *) &buf_size, &buf_size_len);
    printf("send buf size = %d\n", buf_size);

    getsockopt(sock1, SOL_SOCKET, SO_RCVBUF, (char *) &buf_size, &buf_size_len);
    printf("recv buf size = %d\n", buf_size);

    WSACleanup();
}

#endif

int main() {
    sockopt_test();
    //windows_sockopt_test();
    return 0;
}
