#include <cstring>
#include <iostream>
#include <sys/wait.h>
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

//进程间通信
//只要有两个进程可以同时访问的内存空间，就可以通过此空间交换数据。

//通过管道实现进程间通信
//int pipe(int filedes[2]);
void pipe_test() {
    int fds[2];
    char buf[256] = {0};
    char str[] = "hello world";

    pipe(fds);
    pid_t pid = fork();
    if (pid == 0) {
        write(fds[1], str, sizeof(str));
    } else {
        read(fds[0], buf, sizeof(buf));
        printf("%s\n", buf);
    }
}

//通过管道进行进程间双向通信
void pipe_test2() {
    int fds[2];
    int len;
    char buf[256] = {0};
    char str[] = "hello world";
    char str2[] = "hello world you too";

    pipe(fds);
    pid_t pid = fork();
    if (pid == 0) {
        //写数据
        write(fds[1], str, sizeof(str));
        sleep(2);
        //读数据
        len = read(fds[0], buf, sizeof(buf));
        buf[len] = '\0';
        printf("child %s\n", buf);
    } else {
        len = read(fds[0], buf, sizeof(buf));
        buf[len] = '\0';
        printf("parent %s\n", buf);
        write(fds[1], str2, sizeof(str2));
        sleep(2);
    }
}

//通过创建2个管道来完成进程间通信
void pipe_test3() {
    int fds1[2], fds2[2];
    int len;
    char buf[256] = {0};
    char str[] = "hello world";
    char str2[] = "hello world you too";

    pipe(fds1);
    pipe(fds2);

    pid_t pid = fork();
    if (pid == 0) {
        //写数据
        write(fds1[1], str, sizeof(str));
        //读数据
        len = read(fds2[0], buf, sizeof(buf));
        buf[len] = '\0';
        printf("child %s\n", buf);
    } else {
        len = read(fds1[0], buf, sizeof(buf));
        buf[len] = '\0';
        printf("parent %s\n", buf);
        write(fds2[1], str2, sizeof(str2));
    }
}

//保存消息的回声服务器端
void child_exit(int sig) {
    int status;
    pid_t pid = waitpid(-1, &status, 0);
    printf("child exit %d\n", pid);
}

void store_msg_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    struct sigaction act;
    act.sa_handler = child_exit;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGCHLD, &act, NULL);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    int fds[2];
    pipe(fds);
    char buf[256] = {0};
    int len;

    pid_t pid = fork();
    if (pid == 0) {
        FILE *fp = fopen("msg.txt", "wt");
        for (int i = 0; i < 10; i++) {
            //从管道中读取数据
            len = read(fds[0], buf, sizeof(buf));
            fwrite(buf, 1, len, fp);
        }
        fclose(fp);
        exit(0);
    }

    int clt_sock;

    while (1) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
        if (clt_sock == -1) {
            continue;
        }
        pid = fork();
        if (pid == 0) {
            close(sock);

            while ((len = read(clt_sock, buf, sizeof(buf))) > 0) {
                write(clt_sock, buf, len);
                //往管道中写入读到的数据
                write(fds[1], buf, len);
            }

            close(clt_sock);
            exit(0);
        } else {
            close(clt_sock);
        }
    }
    close(sock);
}

void store_msg_client(int port) {
    sockaddr_in addr;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("socket error\n");
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        printf("connect error\n");
        return;
    }

    char buf[256] = {0};
    int len;

    while (1) {
        fgets(buf, sizeof(buf), stdin);

        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
        }

        //这里会重复调用write和read，
        write(sock, buf, strlen(buf));
        len = read(sock, buf, sizeof(buf));
        if (len < 0) {
            printf("read error\n");
            continue;
        }
        buf[len] = '\0';
        printf("msg from server : %s\n", buf);
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    //pipe_test();
    //pipe_test2();
    //pipe_test3();
    if (strcmp(argv[1], "server") == 0) {
        store_msg_server(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        store_msg_client(atoi(argv[2]));
    }
    return 0;
}
