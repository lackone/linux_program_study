#include <cstring>
#include <io.h>
#include <iostream>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//更解UDP
//UDP套接字的特点
//UDP的性能比TCP高出很多，为了提供可靠传输，TCP在不可靠的IP层进行流控制，而UDP缺少这种流控制。
//UDP服务器端/客户端不像TCP那样在连接状态下交换数据，不必调用TCP连接过程中调用的listen和accept，UDP中只有创建套接字的过程和数据交换过程。

//基于UDP的数据I/O函数

//ssize_t sendto(int sock, void *buf, size_t nbytes, int flags, struct sockaddr *to, socklen_t addrlen);
//ssize_t recvfrom(int sock, void *buf, size_t nbytes, int flags, struc sockaddr *from, socklen_t *addrlen);

//UDP不同于TCP，不存在请求连接和受理过程，因此某种意义上无法明确区分服务端和客户端。

#if defined(__linux__) || defined(__unix__)

void udp_echo_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        printf("bind error\n");
        return;
    }

    int len;
    char buf[256] = {0};

    while (1) {
        len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &clt_addr, &clt_addr_len);

        sendto(sock, buf, len, 0, (struct sockaddr *) &clt_addr, clt_addr_len);
    }

    close(sock);
}

void udp_echo_client(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    char buf[256] = {0};
    int len;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (strncmp(buf, "q", 1) == 0 || strncmp(buf, "Q", 1) == 0) {
            break;
        }

        sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &srv_addr, sizeof(srv_addr));

        len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &clt_addr, &clt_addr_len);

        printf("ip %s port %d\n", inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port));

        buf[len] = '\0';

        printf("msg from server: %s\n", buf);
    }

    close(sock);
}

//TCP客户端调用connect函数自动完成IP和端口分配
//UDP在首次调用sendto函数时给相应套接字自动分配IP和端口。

//UDP的数据传输特性
//TCP传输的数据不存在数据边界，数据传输过程中调用I/O函数的次数不具有任何意义。
//相反，UDP是具有数据边界的，输入函数调用次数和输出函数调用次数完全一致，例如，3次输出函数必须调用3次输入函数才能接收完

//UDP套接字传输的数据包又称为数据报，其本身可以成为1个完整数据。UDP中存在数据边界，1个数据包即可成为1个完整数据，因此称为数据报。

void udp_bound_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        printf("bind error\n");
        return;
    }

    char buf[256] = {0};
    int len;

    for (int i = 0; i < 3; i++) {
        sleep(3);
        //这里延时3秒，说明客户端发送的数据已经到了，如果是TCP，只需要一次读取就可以了。
        //但UDP必需要调用3次，才能完整读取
        len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &clt_addr, &clt_addr_len);
        buf[len] = '\0';
        printf("message %d : %s\n", i + 1, buf);
    }
    close(sock);
}

void udp_bound_client(int port) {
    sockaddr_in srv_addr;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    char msg1[] = "hello";
    sendto(sock, msg1, sizeof(msg1), 0, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    char msg2[] = "world";
    sendto(sock, msg2, sizeof(msg2), 0, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    char msg3[] = "!!!";
    sendto(sock, msg3, sizeof(msg3), 0, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    close(sock);
}

//已连接UDP套接字 和 未连接UDP套接字
//通过sendto函数传输数据分为下面3步
//1、向UDP套接字注册目标IP和端口号
//2、传输数据
//3、删除UDP套接字中注册的目标地址信息
//主机82号端口准备了3个数据，调用3次sendto函数，重复上述三步，与同一主机进行长时间通信时，将UDP套接字变成已连接套接字会提高效率。

//创建已连接UDP套接字
//只需要调用 connect 函数，直接使用 write, read函数进行通信。

void udp_connect_echo_client(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    //连接
    connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));

    char buf[256] = {0};
    int len;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (strncmp(buf, "q", 1) == 0 || strncmp(buf, "Q", 1) == 0) {
            break;
        }

        //sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
        write(sock, buf, strlen(buf));

        //len = recvfrom(sock, buf, 256, 0, (struct sockaddr *) &clt_addr, &clt_addr_len);
        read(sock, buf, sizeof(buf));

        buf[len] = '\0';

        printf("msg from server: %s\n", buf);
    }

    close(sock);
}

#else

void windows_udp_echo_server(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN srv_addr, clt_addr;
    int clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        printf("bind error\n");
        return;
    }

    int len;
    char buf[256] = {0};

    while (1) {
        len = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *) &clt_addr, &clt_addr_len);

        sendto(sock, buf, len, 0, (struct sockaddr *) &clt_addr, clt_addr_len);
    }

    closesocket(sock);
    WSACleanup();
}

void windows_udp_echo_client(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN srv_addr, clt_addr;
    int clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    srv_addr.sin_port = htons(port);

    connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));

    char buf[256] = {0};
    int len;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (strncmp(buf, "q", 1) == 0 || strncmp(buf, "Q", 1) == 0) {
            break;
        }

        send(sock, buf, strlen(buf), 0);

        len = recv(sock, buf, sizeof(buf), 0);

        buf[len] = '\0';

        printf("msg from server: %s\n", buf);
    }

    closesocket(sock);
    WSACleanup();
}

#endif

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "server") == 0) {
    //    udp_echo_server(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    udp_echo_client(atoi(argv[2]));
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    udp_bound_server(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    udp_bound_client(atoi(argv[2]));
    //}
    if (strcmp(argv[1], "server") == 0) {
        windows_udp_echo_server(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        windows_udp_echo_client(atoi(argv[2]));
    }
    return 0;
}
