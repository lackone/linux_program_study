#include <iostream>
#include <cstring>

#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/wait.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//并发服务器端的实现方法
//1、多进程服务器  通过创建多个进程提供服务
//2、多路复用服务器 通过捆绑并统一管理I/O对象提供服务
//3、多线程服务器  通过生成与客户端等量的线程提供服务

//理解进程
//占用内存空间的正在运行的程序

//进程ID
//所有进程都会从操作系统分配到ID

//通过fork函数创建进程
//pid_t fork(void);
//fork函数将创建调用的进程副本，两个进程都将执行fork函数调用后的语句。
//因为通过同一进程，复制相同的内存空间，之后的程序要根据fork函数的返回值加以区分。
//父进程：fork函数返回子进程ID
//子进程：fork函数返回0
int g_val = 10;

void fork_test() {
    int l_val = 20;
    g_val++;
    l_val += 5;

    //这里开始创建子进程
    pid_t pid = fork();
    if (pid == 0) {
        //子进程
        g_val += 2;
        l_val += 2;
    } else {
        //父进程
        g_val -= 2;
        l_val -= 2;
    }

    if (pid == 0) {
        printf("child %d %d\n", g_val, l_val);
    } else {
        printf("parent %d %d\n", g_val, l_val);
    }
}

//进程和僵尸进程
//进程完成工作后应被销毁，但有时这些进程将变成僵尸进程，占用系统中的重要资源，这种状态下的进程称作“僵尸进程”

//产生僵尸进程的原因
//传递参数并调用exit函数
//main函数中执行return语句并返回值

//如果父进程未主动要求获得子进程的结束状态值，操作系统将一直保存，并让子进程长时间处于僵尸进程状态。
void zombie_test() {
    pid_t pid = fork();
    if (pid == 0) {
        printf("my child\n");
    } else {
        printf("child %d\n", pid);
        sleep(30);
    }
    //父进程暂停30秒后退出，子进程早早就退出，这时子进程就是僵尸进程
    if (pid == 0) {
        printf("child end\n");
    } else {
        printf("parent end\n");
    }
}

//销毁僵尸进程1：利用wait函数
//为了销毁子进程，父进程应主动请求获取子进程的返回值。
//pid_t wait(int * statloc);
int wait_test() {
    int status;
    pid_t pid = fork();
    if (pid == 0) {
        return 3;
    } else {
        printf("child %d\n", pid);

        pid = fork();
        if (pid == 0) {
            exit(7);
        } else {
            printf("child %d\n", pid);

            //调用wait函数时，如果没有终止的子进程，那么程序将阻塞直到有子进程退出
            wait(&status);
            if (WIFEXITED(status)) {
                printf("%d\n", WEXITSTATUS(status));
            }

            wait(&status);
            if (WIFEXITED(status)) {
                printf("%d\n", WEXITSTATUS(status));
            }

            sleep(10);
        }
    }
    return 0;
}

//销毁僵尸进程2：利用waitpid函数
//pid_t waitpid(pid_t pid, int * statloc, int options);
int wait_test2() {
    int status;
    pid_t pid = fork();
    if (pid == 0) {
        sleep(10);
        return 55;
    } else {
        //-1等待任意子进程退出
        //WNOHANG 没有退出子进程，不阻塞，直接返回0
        while (!waitpid(-1, &status, WNOHANG)) {
            sleep(1);
            printf("sleep 1\n");
        }

        if (WIFEXITED(status)) {
            printf("%d\n", WEXITSTATUS(status));
        }
    }
    return 0;
}

//信号处理
//信号与signal
//void (*signal(int signo, void (*func)(int)))(int);
// SIGALRM   通过调用alarm函数注册的时间
// SIGINT    输入ctrl+c
// SIGCHLD   子进程退出

//unsigned int alarm(unsigned int seconds);
//传递一个正整数型参数，相应时间后，将产生SIGALRM信号。

void timeout(int sig) {
    if (sig == SIGALRM) {
        printf("timeout\n");
    }
    alarm(2);
}

void keypress(int sig) {
    if (sig == SIGINT) {
        printf("ctrl+c \n");
    }
}

void signal_test() {
    signal(SIGALRM, timeout);
    signal(SIGINT, keypress);
    alarm(2); //2秒后触发信号

    //产生信号时，为了调用信号处理函数，将唤醒由于调用sleep函数而进入阻塞状态的进程
    //进程唤醒，就不会再进入睡眠状态，即使还未到sleep函数中规定的时间
    for (int i = 0; i < 3; i++) {
        printf("wait...\n");
        sleep(10);
    }
}

//利用 sigaction 函数处理信号
//int sigaction(int signo, const struct sigaction *act, struct sigaction *oldact);
//struct sigaction {
//  void (*sa_handler)(int);
//  sigset_t sa_mask;
//  int sa_flags;
//};

void signal_test2() {
    struct sigaction act;
    act.sa_handler = timeout;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    sigaction(SIGALRM, &act, NULL);

    alarm(2);

    for (int i = 0; i < 3; i++) {
        printf("wait...\n");
        sleep(10);
    }
}

//利用信号处理技术消灭僵尸进程

void child_end(int sig) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (WIFEXITED(status)) {
        printf("child id %d\n", pid);
        printf("child exit %d\n", WEXITSTATUS(status));
    }
}

int remove_zombie() {
    struct sigaction act;
    act.sa_handler = child_end;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGCHLD, &act, NULL);

    pid_t pid = fork();
    if (pid == 0) {
        printf("child\n");
        sleep(10);
        return 22;
    } else {
        printf("child %d\n", pid);
        pid = fork();
        if (pid == 0) {
            printf("child\n");
            sleep(10);
            exit(33);
        } else {
            printf("child %d\n", pid);

            for (int i = 0; i < 10; i++) {
                printf("wait...\n");
                sleep(10);
            }
        }
    }
    return 0;
}

//多进程并发服务器
//1、回声服务器端（父进程），通过调用accept函数受理连接请求
//2、此时获取的套接字文件描述符创建并传递给子进程
//3、子进程利用传递来的文件描述符提供服务
void child_exit(int sig) {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    printf("child exit %d\n", pid);
}

void multi_process_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    struct sigaction act;
    act.sa_handler = child_exit;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGCHLD, &act, NULL);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("socket error\n");
        return;
    }
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr)) < 0) {
        printf("bind error\n");
        return;
    }
    if (listen(sock, 5) < 0) {
        printf("listen error\n");
        return;
    }

    int clt_sock;
    pid_t pid;
    int len;
    char buf[256] = {0};

    while (1) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);
        if (clt_sock < 0) {
            continue;
        }
        pid = fork();
        if (pid == -1) {
            close(clt_sock);
            continue;
        }
        if (pid == 0) {
            //父进程将 sock 和 clt_sock 复制一份给了子进程
            //子进程
            close(sock);

            while ((len = read(clt_sock, buf, sizeof(buf))) > 0) {
                write(clt_sock, buf, len);
            }

            close(clt_sock);
            printf("client exit\n");
            exit(0);
        } else {
            close(clt_sock);
        }
    }
    close(sock);
}

void multi_process_client(int port) {
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

//分割TCP的I/O程序
//将程序的读与写，通过父进程与子进程进行分开处理
void read_routine(int sock, char *buf, int buf_len) {
    while (1) {
        int len = read(sock, buf, buf_len);
        if (len < 0) {
            return;
        }
        buf[len] = '\0';
        printf("msg from server : %s\n", buf);
    }
}

void write_routine(int sock, char *buf, int buf_len) {
    while (1) {
        fgets(buf, buf_len, stdin);
        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            //半关闭写输出
            //向服务器端传递EOF
            shutdown(sock, SHUT_WR);
            return;
        }
        write(sock, buf, strlen(buf));
    }
}

void multi_process_client2(int port) {
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
    pid_t pid = fork();

    if (pid == 0) {
        //子进程写
        write_routine(sock, buf, sizeof(buf));
    } else {
        //父进程读
        read_routine(sock, buf, sizeof(buf));
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    //fork_test();
    //return wait_test();
    //return wait_test2();
    //signal_test();
    //signal_test2();
    //return remove_zombie();
    if (strcmp(argv[1], "server") == 0) {
        multi_process_server(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        //multi_process_client(atoi(argv[2]));
        multi_process_client2(atoi(argv[2]));
    }
    return 0;
}
