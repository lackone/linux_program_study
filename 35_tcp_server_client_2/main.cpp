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

#if defined(__linux__) || defined(__unix__)

//回声客户端的问题解决方法
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
    int str_len;
    int recv_len;

    while (1) {
        fgets(msg, sizeof(msg), stdin);

        if (strncmp(msg, "q", 1) == 0 ||
            strncmp(msg, "Q", 1) == 0) {
            break;
        }

        str_len = write(sock, msg, strlen(msg));
        recv_len = 0;

        while (recv_len < str_len) {
            //循环判断接收的数据长度
            len = read(sock, &msg[recv_len], sizeof(msg));
            if (len == -1) {
                break;
            }
            recv_len += len;
        }

        msg[recv_len] = '\0';

        printf("msg from server %s\n", msg);
    }

    close(sock);
}

void op_client_test(int port) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("connect error\n");
        return;
    }
    int op_cnt;
    char op_msg[256] = {0};
    fputs("operand count:", stdout);
    scanf("%d", &op_cnt);

    op_msg[0] = (char) op_cnt; //操作数个数

    for (int i = 0; i < op_cnt; i++) {
        printf("operand %d:", i + 1);
        scanf("%d", &op_msg[i * sizeof(int) + 1]);
    }

    fgetc(stdin);
    fputs("operand:", stdout);
    scanf("%c", &op_msg[op_cnt * sizeof(int) + 1]);

    //op_msg 内存 1字节操作个数 + 4字节*操作个数 + 1字节操作数
    write(sock, op_msg, op_cnt * sizeof(int) + 2);

    //结果4字节
    int ret;
    read(sock, &ret, sizeof(int));

    printf("operation result: %d\n", ret);

    close(sock);
}

int calc(int op_cnt, int opnds[], char op) {
    int ret = opnds[0];
    switch (op) {
        case '+':
            for (int i = 1; i < op_cnt; i++) {
                ret += opnds[i];
            }
            break;
        case '-':
            for (int i = 1; i < op_cnt; i++) {
                ret -= opnds[i];
            }
            break;
        case '*':
            for (int i = 1; i < op_cnt; i++) {
                ret *= opnds[i];
            }
            break;
        case '/':
            for (int i = 1; i < op_cnt; i++) {
                ret /= opnds[i];
            }
            break;
    }
    return ret;
}

void op_server_test(int port) {
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
    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) == -1) {
        printf("bind error\n");
        return;
    }
    if (listen(sock, 5) == -1) {
        printf("listen error\n");
        return;
    }

    int recv_len;
    int len;
    int clt_sock;
    char op_cnt;
    char op_msg[256] = {0};
    int ret;

    for (int i = 0; i < 5; i++) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
        if (clt_sock == -1) {
            printf("accept error\n");
            continue;
        }
        //读取一字节
        read(clt_sock, &op_cnt, 1);
        printf("op_cnt = %d\n", op_cnt);
        recv_len = 0;
        while (recv_len < (op_cnt * sizeof(int) + 1)) {
            len = read(clt_sock, &op_msg[recv_len], sizeof(op_msg));
            recv_len += len;
        }
        ret = calc(op_cnt, (int *) op_msg, op_msg[recv_len - 1]);
        write(clt_sock, &ret, sizeof(int));
        close(clt_sock);
    }

    close(sock);
}

//TCP套接字中的I/O缓冲
//I/O缓冲在每个TCP套接字中单独存在
//I/O缓冲在创建套接字时自动生成
//即使关闭套接字也会继续传递输出缓冲击中遗留的数据
//关闭套接字将丢失输入缓冲中的数据

//TCP内部工作原理
//TCP在实际通信过程中，会经过3次对话过程，该过程又称为Three-way handshaking(三次握手)
//套接字是以全双工方式工作的，可以双向传递数据。

//1、与对方套接字的连接
// 客户端 A =》 客户端 B
// SYN  SEQ:1000 ACK:-   数据包序号1000，如果接收无误，通知我向你传递1001号数据包。
// 客户端 B =》 客户端 A
// SYN+ACK  SEQ:2000 ACK:1001   数据包序号2000，如果接收无误，通知我向你传递2001号数据包，刚才传输的1000数据包接收无误，请传递SEQ=1001数据包
// 客户端 A =》 客户端 B
// ACK  SEQ:1001 ACK:2001   确认收到2000数据包，可以传输2001的数据包

//2、与对方主机的数据交换
// 客户端 A =》 客户端 B
// SEQ:1200 100 byte data  传输100字节数据
// 客户端 B =》 客户端 A
// ACK:1301                确认收到
// 客户端 A =》 客户端 B
// SEQ:1301 100 byte data  传输100字节数据
// 客户端 B =》 客户端 A
// ACK:1402                确认收到

//ACK号 =》SEQ号 + 传递的字节数 + 1

//3、断开与套接字的连接
// 客户端 A =》 客户端 B
// FIN SEQ:5000 ACK:-   A向B发送断开连接的请求
// 客户端 B =》 客户端 A
// ACK SEQ:7000 ACK:5001   B确认收到请求
// 客户端 B =》 客户端 A
// FIN SEQ:7001 ACK:5001   B向A发送断开连接请求
// 客户端 A =》 客户端 B
// ACK SEQ:5001 ACK:7002   A确认收到消息

#else

void windows_op_client_test(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("socket error\n");
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        printf("connect error\n");
        return;
    }
    int op_cnt;
    char op_msg[256] = {0};
    fputs("operand count:", stdout);
    scanf("%d", &op_cnt);

    op_msg[0] = (char) op_cnt; //操作数个数

    for (int i = 0; i < op_cnt; i++) {
        printf("operand %d:", i + 1);
        scanf("%d", &op_msg[i * sizeof(int) + 1]);
    }

    fgetc(stdin);
    fputs("operand:", stdout);
    scanf("%c", &op_msg[op_cnt * sizeof(int) + 1]);

    //op_msg 内存 1字节操作个数 + 4字节*操作个数 + 1字节操作数
    send(sock, op_msg, op_cnt * sizeof(int) + 2, 0);

    //结果4字节
    int ret;
    recv(sock, (char *) &ret, sizeof(int), 0);

    printf("operation result: %d\n", ret);

    closesocket(sock);
    WSACleanup();
}

int windows_calc(int op_cnt, int opnds[], char op) {
    int ret = opnds[0];
    switch (op) {
        case '+':
            for (int i = 1; i < op_cnt; i++) {
                ret += opnds[i];
            }
            break;
        case '-':
            for (int i = 1; i < op_cnt; i++) {
                ret -= opnds[i];
            }
            break;
        case '*':
            for (int i = 1; i < op_cnt; i++) {
                ret *= opnds[i];
            }
            break;
        case '/':
            for (int i = 1; i < op_cnt; i++) {
                ret /= opnds[i];
            }
            break;
    }
    return ret;
}

void windows_op_server_test(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN srv_addr, clt_addr;
    int clt_addr_len = sizeof(clt_addr);

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

    int recv_len;
    int len;
    int clt_sock;
    char op_cnt;
    char op_msg[256] = {0};
    int ret;

    for (int i = 0; i < 5; i++) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
        if (clt_sock == -1) {
            printf("accept error\n");
            continue;
        }
        //读取一字节
        recv(clt_sock, &op_cnt, 1, 0);
        printf("op_cnt = %d\n", op_cnt);
        recv_len = 0;
        while (recv_len < (op_cnt * sizeof(int) + 1)) {
            len = recv(clt_sock, &op_msg[recv_len], sizeof(op_msg), 0);
            recv_len += len;
        }
        ret = windows_calc(op_cnt, (int *) op_msg, op_msg[recv_len - 1]);
        send(clt_sock, (char *) &ret, sizeof(int), 0);
        closesocket(clt_sock);
    }

    closesocket(sock);
    WSACleanup();
}
#endif

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "server") == 0) {
    //    op_server_test(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    op_client_test(atoi(argv[2]));
    //}
    if (strcmp(argv[1], "server") == 0) {
        windows_op_server_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        windows_op_client_test(atoi(argv[2]));
    }
    return 0;
}
