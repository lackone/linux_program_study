#include <cstring>
#include <iostream>
#include <stdio.h>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//基于TCP的半关闭
//单方面断开连接带来的问题
//close函数和closesocket函数意味着完全断开连接，无法传输数据，也不能接收数据。某些情况下，一方调用close或closesocket函数断开连接就显得不优雅。
//只关闭一部分数据交换中使用的流的方法应运而生。断开一部分连接指，可传输但无法接收，可接收但无法传输，只关闭流的一半。

//套接字和流stream
//2台主机通过套接字连接后，进入交换数据状态，也称流形成的状态。把建立套接字后可交换数据的状态看作一种流。
//水朝着一个方向流动，数据也只能向一个方向移动，为了进行双向通信，需要2个流。
//2台主机建立了连接，主机会拥有单独的输入流和输出流，其中一个主机的输入流与另一个主机的输出流相连，而输出流与另一主机的输入流相连。
//close和closesocket函数将同时断开这2个流。

//int shutdown(int sock, int howto);
//SHUT_RD 断开输入
//SHUT_WR 断开输出
//SHUT_RDWR 同时断开I/O流

//基于半关闭的文件传输程序

#if defined(__linux__) || defined(__unix__)

void file_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    FILE *fp = fopen("1.txt", "rb");
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

    char buf[32] = {0};
    int len;

    //循环读取文件，并发送
    while (1) {
        len = fread(buf, 1, sizeof(buf), fp);
        printf("sizeof(buf) %d\n", sizeof(buf));
        printf("len: %d\n", len);
        if (len < sizeof(buf)) {
            write(clt_sock, buf, len);
            break;
        }
        write(clt_sock, buf, len);
    }

    shutdown(clt_sock, SHUT_WR); //关闭输出流，这样就向客户端传输了EOF
    len = read(clt_sock, buf, sizeof(buf)); //仍然可以读取数据
    buf[len] = '\0';
    printf("msg from client: %s\n", buf);

    fclose(fp);
    close(clt_sock);
    close(sock);
}

void file_client(int port) {
    sockaddr_in srv_addr;

    FILE *fp = fopen("2.txt", "wb");
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));

    int len;
    char buf[32] = {0};

    while ((len = read(sock, buf, sizeof(buf))) > 0) {
        printf("len: %d\n", len);
        fwrite(buf, 1, len, fp);
    }

    printf("received file data\n");
    write(sock, "ok", 2);
    fclose(fp);
    close(sock);
}

#else

//基于windows的实现
//int shutdown(SOCKET sock, int howto);
//SD_RECEIVE  断开输入
//SD_SEND  断开输出
//SD_BOTH  同时断开

void windows_file_server(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN srv_addr, clt_addr;
    int clt_addr_len = sizeof(clt_addr);

    FILE *fp = fopen("1.txt", "rb");
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

    char buf[32] = {0};
    int len;

    //循环读取文件，并发送
    while (1) {
        len = fread(buf, 1, sizeof(buf), fp);
        printf("sizeof(buf) %d\n", sizeof(buf));
        printf("len: %d\n", len);
        if (len < sizeof(buf)) {
            send(clt_sock, buf, len, 0);
            break;
        }
        send(clt_sock, buf, len, 0);
    }

    shutdown(clt_sock, SD_SEND); //关闭输出流，这样就向客户端传输了EOF
    len = recv(clt_sock, buf, sizeof(buf), 0); //仍然可以读取数据
    buf[len] = '\0';
    printf("msg from client: %s\n", buf);

    fclose(fp);
    closesocket(clt_sock);
    closesocket(sock);
    WSACleanup();
}

void windows_file_client(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }

    SOCKADDR_IN srv_addr;

    FILE *fp = fopen("2.txt", "wb");
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srv_addr.sin_port = htons(port);

    connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));

    int len;
    char buf[32] = {0};

    while ((len = recv(sock, buf, sizeof(buf), 0)) > 0) {
        printf("len: %d\n", len);
        fwrite(buf, 1, len, fp);
    }

    printf("received file data\n");
    send(sock, "ok", 2, 0);
    fclose(fp);
    closesocket(sock);
    WSACleanup();
}

#endif

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "server") == 0) {
    //    file_server(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    file_client(atoi(argv[2]));
    //}
    if (strcmp(argv[1], "server") == 0) {
        windows_file_server(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        windows_file_client(atoi(argv[2]));
    }
    return 0;
}
