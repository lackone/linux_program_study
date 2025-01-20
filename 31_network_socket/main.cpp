#include <cstring>
#include <io.h>
#include <iostream>

#if defined(__linux__) || defined(__unix__)
    #include <unistd.h>
    #include <netinet/in.h>
    #include <fcntl.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

#if defined(__linux__) || defined(__unix__)
//构建接电话套接字
//1、调用socket函数（安装电话机）时进行的对话
//2、调用bind函数（分配电话号码）时进行的对话
//3、调用listen（连接电话线）时进行的对话
//4、调用accept函数（拿起话筒）时进行的对话

void hello_world_server(int port) {
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

void hello_world_client(int port) {
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
    int len = read(fd, msg, sizeof(msg));
    if (len < 0) {
        perror("read error");
        return;
    }

    msg[len] = '\0';
    printf("msg from server : %s\n", msg);
    close(fd);
}

//基于linux的文件操作
//在linux世界里，socket也被认为是文件的一种，因此网络数据传输过程中自然可以使用文件I/O的相关函数。

//底层文件访问 和 文件描述符
//打开文件
//int open(const char *path, int flag);
//关闭文件
//int close(int fd);
//写入文件
//ssize_t write(int fd, const void* buf, size_t nbytes);
//读取文件
//ssize_t read(int fd, void *buf, size_t nbytes);

void file_test() {
    int fd = open("data.txt", O_CREAT | O_WRONLY | O_TRUNC, 0664);
    if (fd < 0) {
        perror("open error");
        return;
    }
    char msg[] = "Hello World!\n";
    if (write(fd, msg, strlen(msg)) < 0) {
        perror("write error");
        return;
    }
    close(fd);
}

void file_read_test() {
    int fd = open("data.txt", O_RDONLY);
    if (fd < 0) {
        perror("open error");
        return;
    }
    char msg[256] = {0};
    if (read(fd, msg, sizeof(msg)) < 0) {
        perror("read error");
        return;
    }
    printf("file data : %s\n", msg);
    close(fd);
}

//文件描述符与套接字
void file_socket_fd_test() {
    int fd1 = socket(AF_INET, SOCK_STREAM, 0);
    int fd2 = open("test.dat", O_CREAT | O_WRONLY | O_TRUNC, 0664);
    int fd3 = socket(AF_INET, SOCK_DGRAM, 0);

    //描述符，从小到大
    printf("fd1: %d\n", fd1);
    printf("fd2: %d\n", fd2);
    printf("fd3: %d\n", fd3);

    close(fd1);
    close(fd2);
    close(fd3);
}

#else

//widows套接字编程
//1、导入头文件 winsock2.h
//2、链接 ws2_32.lib 库
//3、在 CMakeLists.txt 中添加  target_link_libraries(31_network_socket ws2_32)

//winsock 初始化
//int WSAStartup(WORD wVersionRequested,LPWSADATA lpWSAData);
//winsock 注销
//int WSACleanup(void);
//创建套接字
//SOCKET socket(int af,int type,int protocol);
//绑定
//int bind(SOCKET s,const struct sockaddr *name,int namelen);
//监听
//int listen(SOCKET s,int backlog);
//获取客户连接
//SOCKET accept(SOCKET s,struct sockaddr *addr,int *addrlen);
//连接
//int connect(SOCKET s,const struct sockaddr *name,int namelen);
//关闭套接字
//int closesocket(SOCKET s);

void windows_server_test(int port) {
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

void windows_client_test(int port) {
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
    int len = recv(sock, msg, sizeof(msg), 0);
    if (len < 0) {
        printf("recv error\n");
        return;
    }

    msg[len] = '\0';
    printf("msg from server : %s\n", msg);

    closesocket(sock);
    WSACleanup();
}

#endif

//基于windows的I/O函数
//windows严格区分文件I/O函数和套接字I/O函数
//发送数据
//int send(SOCKET s,const char *buf,int len,int flags);
//接收数据
//int recv(SOCKET s,char *buf,int len,int flags);

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "server") == 0) {
    //    hello_world_server(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    hello_world_client(atoi(argv[2]));
    //}
    //file_test();
    //file_read_test();
    //file_socket_fd_test();
    if (strcmp(argv[1], "server") == 0) {
        windows_server_test(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        windows_client_test(atoi(argv[2]));
    }
    return 0;
}
