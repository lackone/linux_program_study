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

//标准IO函数的优点
//标准IO函数具有良好的移植性
//标准IO函数可以利用缓冲提高性能
void read_write_test() {
    int fd1 = open("data.txt", O_RDONLY);
    int fd2 = open("data.txt", O_RDWR | O_CREAT | O_TRUNC);

    int len;
    char buf[256] = {0};

    while ((len = read(fd1, buf, sizeof(buf))) > 0) {
        write(fd2, buf, len);
    }

    close(fd1);
    close(fd2);
}

void fread_fwrite_test() {
    FILE *fp1 = fopen("data.txt", "r");
    FILE *fp2 = fopen("data.txt", "w");

    char buf[256] = {0};

    while (fgets(buf, sizeof(buf), fp1)) {
        fputs(buf, fp2);
    }
    fclose(fp1);
    fclose(fp2);
}

//使用标准IO函数
//利用fdopen函数转换为FILE结构体指针
//FILE* fdopen(int fildes, const char *mode);
void fdopen_test() {
    int fd = open("data.txt", O_CREAT | O_RDWR | O_TRUNC);

    FILE *fp = fdopen(fd, "w");
    fputs("hello,world\n", fp);
    fclose(fp);
}

//利用fileno函数转换为文件描述符
//int fileno(FILE *stream);
void fileno_test() {
    FILE *fp = fopen("data.txt", "w+");
    int fd = fileno(fp);
    write(fd, "test", strlen("test"));
    close(fd);
}

//基于套接字的标准IO函数使用
void echo_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);
    int clt_sock;
    FILE *rfp;
    FILE *wfp;
    char buf[256] = {0};
    for (int i = 0; i < 5; i++) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
        rfp = fdopen(clt_sock, "r");
        wfp = fdopen(clt_sock, "w");
        while (!feof(rfp)) {
            fgets(buf, sizeof(buf), rfp);
            fputs(buf, wfp);
            fflush(wfp);
        }
        fclose(rfp);
        fclose(wfp);
    }
    close(sock);
}

void echo_client(int port) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    FILE *rfp = fdopen(sock, "r");
    FILE *wfp = fdopen(sock, "w");
    char buf[256] = {0};
    int len;
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
        }
        fputs(buf, wfp);
        fflush(wfp);
        fgets(buf, sizeof(buf), rfp);
        printf("msg from server :%s\n", buf);
    }
    fclose(rfp);
    fclose(wfp);
}

int main(int argc, char *argv[]) {
    //fdopen_test();
    //fileno_test();
    if (strcmp(argv[1], "server") == 0) {
        echo_server(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        echo_client(atoi(argv[2]));
    }
    return 0;
}
