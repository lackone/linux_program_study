#include <cstring>
#include <iostream>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//TCP是 Transmission Control Protoco 传输控制协议的简写，意为对数据传输过程的控制。

//链路层
//链路层是物理链接领域标准化的结果，专门定义LAN，WAN，MAN等网络标准。

//IP层
//IP本身是面向消息的，不可靠的协议，每次传输数据时会帮我们选择路径。IP协议无法应对数据错误。

//TCP/UDP层
//TCP可以保证可靠的数据传输

//应用层

#if defined(__linux__) || defined(__unix__)
void tcp_server_test(int port) {
    sockaddr_in srv_addr;
    sockaddr_in clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(sock, 5) == -1) {
        printf("listen error\n");
        return;
    }

    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
    if (clt_sock == -1) {
        printf("accept error\n");
        return;
    }

    char msg[] = "Hello World!\n";
    write(clt_sock, msg, sizeof(msg));

    close(clt_sock);
    close(sock);
}

void tcp_client_test(int port) {
    sockaddr_in srv_addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        printf("connect error\n");
        return;
    }

    char msg[256] = {0};
    int len = read(sock, msg, sizeof(msg));
    if (len == -1) {
        printf("read error\n");
        return;
    }

    msg[len] = '\0';
    printf("msg from server %s\n", msg);
    close(sock);
}

//实现迭代服务器端/客户端
void echo_server_test(int port) {
    sockaddr_in srv_addr;
    sockaddr_in clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(sock, 5) == -1) {
        printf("listen error\n");
        return;
    }

    int clt_sock;
    int len;
    char msg[256] = {0};

    //处理5个客户端连接
    for (int i = 0; i < 5; i++) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
        if (clt_sock == -1) {
            printf("accept error\n");
            continue;
        }

        while (len = read(clt_sock, msg, sizeof(msg))) {
            write(clt_sock, msg, len);
        }

        close(clt_sock);
    }

    close(sock);
}

void echo_client_test(int port) {
    sockaddr_in srv_addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        printf("connect error\n");
        return;
    }

    char msg[256] = {0};
    int len;

    while (1) {
        fgets(msg, sizeof(msg), stdin);

        if (strncmp(msg, "q", 1) == 0 ||
            strncmp(msg, "Q", 1) == 0) {
            break;
        }

        write(sock, msg, strlen(msg));
        len = read(sock, msg, sizeof(msg));
        msg[len] = '\0';

        printf("msg from server %s\n", msg);
    }
}

#else

//基于windows的回声服务器端
//将linux转化成windows平台示例，记住以下
//1、通过 WSAStartup、WSACleanup函数初始化并清除套接字相关库
//2、把数据类型和变量名切换为windows风格
//3、数据传输中用recv，send函数而非read，write函数
//4、关闭套接字时用closesocket函数而非close函数

void echo_windows_server_test(int port) {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN srv_addr, clt_addr;
    int clt_addr_len = sizeof(clt_addr);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket error\n");
        return;
    }
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == SOCKET_ERROR) {
        printf("bind error\n");
        return;
    }

    if (listen(sock, 5) == SOCKET_ERROR) {
        printf("listen error\n");
        return;
    }

    int clt_sock;
    char msg[256] = {0};
    int len;

    for (int i = 0; i < 5; i++) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
        if (clt_sock == INVALID_SOCKET) {
            printf("accept error\n");
            continue;
        }
        while (len = recv(clt_sock, msg, sizeof(msg), 0)) {
            send(clt_sock, msg, len, 0);
        }

        closesocket(clt_sock);
    }

    closesocket(sock);
    WSACleanup();
}

void echo_windows_client_test(int port) {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("socket error\n");
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("connect error\n");
        return;
    }

    char msg[256] = {0};
    int len;

    while (1) {
        fgets(msg, sizeof(msg), stdin);

        if (strncmp(msg, "q", 1) == 0 ||
            strncmp(msg, "Q", 1) == 0) {
            break;
        }

        send(sock, msg, strlen(msg), 0);
        len = recv(sock, msg, sizeof(msg), 0);
        msg[len] = '\0';

        printf("msg from server %s\n", msg);
    }

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
    //if (strcmp(argv[1], "server") == 0) {
    //    echo_server_test(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    echo_client_test(atoi(argv[2]));
    //}
    if (strcmp(argv[1], "server") == 0) {
        echo_windows_server_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        echo_windows_client_test(atoi(argv[2]));
    }
    return 0;
}
