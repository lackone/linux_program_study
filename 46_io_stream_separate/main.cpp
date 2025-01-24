#include <cstring>
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

//分离IO流

//2次IO流分离
//1、第一种，TCP I/O过程分离，通过fork函数复制出1个文件描述符，以区分输入和输出
//2、通过2次fdopen函数，创建读模式FILE指针和写模式FILE指针

//流分离带来的EOF问题
void sep_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);
    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

    FILE *rfp = fdopen(clt_sock, "r");
    FILE *wfp = fdopen(clt_sock, "w");
    fputs("hello world!\n", wfp);
    fputs("test\n", wfp);
    fputs("are you ok\n", wfp);
    fflush(wfp);
    fclose(wfp); //直接终止了套接字，不是半关闭

    char buf[256] = {0};
    //这里并没有收到客户端的消息
    fgets(buf, sizeof(buf), rfp);
    fputs(buf, stdout);
    fclose(rfp);
}

void sep_client(int port) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    FILE *rfp = fdopen(sock, "r");
    FILE *wfp = fdopen(sock, "w");

    char buf[256] = {0};
    while (1) {
        if (fgets(buf, sizeof(buf), rfp) == NULL) {
            break;
        }
        fputs(buf, stdout);
        fflush(stdout);
    }

    fputs("from client ok\n", wfp);
    fflush(wfp);
    fclose(wfp);
    fclose(rfp);
}

//如何针对FILE的半关闭
//因为读FILE指针与写FILE指针，基于同一文件描述符，只要一方调用fclose，都会关闭文件描述符
//解决方法，复制一份文件描述符，各自生成FILE指针，销毁所有文件描述符后才能销毁套接字。

//复制文件描述符
//int dup(int fildes);
//int dup2(int fildes, int fildes2);
void dup_test() {
    int c1 = dup(1); //输出
    int c2 = dup2(c1, 7); //指定文件描述符整数值

    printf("c1 = %d\n", c1);
    printf("c2 = %d\n", c2);

    write(c1, "hello\n", strlen("hello\n"));
    write(c2, "world\n", strlen("world\n"));

    close(c1);
    close(c2);
    write(1, "test\n", strlen("test\n"));
    close(1); //完全关闭输出
    write(1, "ok\n", strlen("ok\n"));
}


void sep_server2(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);
    int clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

    //复制文件描述符
    int clt_sock_cp = dup(clt_sock);
    //分别
    FILE *rfp = fdopen(clt_sock_cp, "r");
    FILE *wfp = fdopen(clt_sock, "w");
    fputs("hello world!\n", wfp);
    fputs("test\n", wfp);
    fputs("are you ok\n", wfp);
    fflush(wfp);

    shutdown(fileno(wfp), SHUT_WR);
    fclose(wfp); //这里就相当于半关闭了

    char buf[256] = {0};
    //可以收到消息了
    fgets(buf, sizeof(buf), rfp);
    fputs(buf, stdout);
    fclose(rfp);
}

int main(int argc, char *argv[]) {
    if (strcmp(argv[1], "server") == 0) {
        //sep_server(atoi(argv[2]));
        sep_server2(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        sep_client(atoi(argv[2]));
    }
    //dup_test();
    return 0;
}
