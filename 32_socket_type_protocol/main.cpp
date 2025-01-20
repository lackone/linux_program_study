#include <iostream>
#include <cstring>
#include <io.h>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

//协议族
//PF_INET   ipv4协议族
//PF_INET6  ipv6协议族
//PF_LOCAL  本地通信的unix协议族
//PF_PACKET 底层套按字的协议族
//PF_IPX    IPX novell协议族

//套接字类型
//1、面向连接的套接字  SOCK_STREAM
//  传输过程中数据不会消失
//  按序传输数据
//  传输的数据不存在数据边界(boundary)
//  总结：可靠的，按序传递的，基于字节的面向连接的数据传输方式的套接字
//2、面向消息的套接字  SOCK_DGRAM
//  强调快速传输而非传输顺序
//  传输的数据可能丢失也可能损毁
//  传输的数据有数据边界
//  限制每次传输的数据大小
//  总结：不可靠的，不按序传递的，以数据的高速传输为目的的套接字

#if defined(__linux__) || defined(__unix__)

void tcp_server_test(int port) {
    sockaddr_in ser_addr;
    sockaddr_in clt_addr;
    socklen_t clt_len;

    int ser_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ser_fd < 0) {
        perror("socket error");
        return;
    }

    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(port);

    if (bind(ser_fd, (sockaddr *) &ser_addr, sizeof(ser_addr)) < 0) {
        perror("bind error");
        return;
    }

    if (listen(ser_fd, 5) < 0) {
        perror("listen error");
        return;
    }

    clt_len = sizeof(clt_addr);
    int clt_fd = accept(ser_fd, (sockaddr *) &clt_addr, &clt_len);
    if (clt_fd < 0) {
        perror("accept error");
        return;
    }

    char msg[] = "Hello World!\n";
    write(clt_fd, msg, sizeof(msg));

    close(clt_fd);
    close(ser_fd);
}

void tcp_client_test(int port) {
    sockaddr_in addr;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket error");
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (connect(fd, (sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("connect error");
        return;
    }

    char msg[256] = {0};
    int len;
    int idx = 0;
    int total = 0;

    //循环读取，每次1字节
    //传输的数据不存在数据边界
    while (len = read(fd, &msg[idx++], 1)) {
        if (len == -1) {
            perror("read error");
            return;
        }
        total += len;
    }

    printf("msg from server : %s\n", msg);
    printf("read call count : %d\n", total);
    close(fd);
}

#else

void windows_tcp_server_test(int port) {
    WSADATA wsaData;
    SOCKET ser_sock, clt_sock;
    SOCKADDR_IN ser_addr, clt_addr;
    int clt_addr_len;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }

    ser_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (ser_sock == INVALID_SOCKET) {
        printf("socket error\n");
        return;
    }

    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    ser_addr.sin_port = htons(port);

    if (bind(ser_sock, (SOCKADDR *) &ser_addr, sizeof(ser_addr)) == SOCKET_ERROR) {
        printf("bind error\n");
        return;
    }

    if (listen(ser_sock, 5) == SOCKET_ERROR) {
        printf("listen error\n");
        return;
    }

    clt_addr_len = sizeof(clt_addr);
    clt_sock = accept(ser_sock, (SOCKADDR *) &clt_addr, &clt_addr_len);
    if (clt_sock == INVALID_SOCKET) {
        printf("accept error\n");
        return;
    }

    char msg[] = "Hello World!\n";
    send(clt_sock, msg, sizeof(msg), 0);

    closesocket(clt_sock);
    closesocket(ser_sock);

    WSACleanup();
}

void windows_tcp_client_test(int port) {
    printf("%d\n", port);

    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket error\n");
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (SOCKADDR *) &addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("connect error %d\n", WSAGetLastError());
        return;
    }

    char msg[256] = {0};
    int len;
    int idx = 0;
    int total = 0;
    while (len = recv(sock, &msg[idx++], 1, 0)) {
        if (len == -1) {
            printf("recv error\n");
            return;
        }
        total += len;
    }

    printf("msg from server : %s\n", msg);
    printf("recv call count : %d\n", total);

    closesocket(sock);
    WSACleanup();
}

#endif

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "server") == 0) {
    //    tcp_server_test(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    tcp_client_test(atoi(argv[2]));
    //}

    if (strcmp(argv[1], "server") == 0) {
        windows_tcp_server_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        windows_tcp_client_test(atoi(argv[2]));
    }
    return 0;
}
