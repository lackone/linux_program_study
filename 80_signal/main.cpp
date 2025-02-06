#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

using namespace std;

//信号
//信号是由用户，系统，进程发送给目标进程的信息，以通知目标进程某个状态的改变或系统异常

//发送信号
//int kill(pid_t pid, int sig);
//pid>0，信号发送给指定进程
//pid=0，信号发送给本进程组内的其他进程
//pid=-1，信号发送给除init进程外的所有进程
//pid<-1，信号发送给组ID为-pid的进程组中的所有成员

//信号处理
//typedef void (*__sighandler_t)(int);

//define SIG_DEF ((__sighandler_t) 0)  表示使用信号的默认处理方式
//define SIG_IGN ((__sighandler_t) 1)  表示忽略目标信号

//linux信号
//linux的可用信号都定义在bits/signum.h头文件中
//重点 SIGHUP SIGPIPE SIGURG

//信号函数
//_sighandler_t signal(int sig, _sighandler_t _handler);
//int sigaction(int sig, const struct sigaction* act, struct sigaction* oact);

//信号集
//typedef struct {
//    unsigned long int __val[_SIGSET_NWORDS];
//} __sigset_t;
//int sigemptyset(sigset_t* set); //清空信号集
//int sigfillset(sigset_t* set); //在信号集中设置所有信号
//int sigaddset(sigset_t* set, int _signo); //添加信号
//int sigdelset(sigset_t* set, int _signo); //删除信号
//int sigismember(const sigset_t* set, int _signo); //判断信号是否存在

//进程信号掩码
//int sigprocmask(int _how, _const sigset_t*　_set, sigset_t* _oset);

//被挂起的信号
//设置进程信号掩码后，屏蔽的信号将不能被接收，给进程发送一个屏蔽信号，系统将该信号设置为进程的一个被挂起的信号，如果取消被挂起信号，则立即被进程接收到。
//int sigpending(sigset_t* set); //获取当前进程挂起的信号集

//统一事件源
int setnonblocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void addfd(int epfd, int fd) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

int sig_pipefd[2];

//信号处理函数
void sig_handler(int sig) {
    //保留上一次errno
    int pre_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], &msg, 1, 0); //向管道写入信号
    errno = pre_errno;
}

void add_sig(int sig) {
    struct sigaction act;
    act.sa_handler = sig_handler;
    act.sa_flags |= SA_RESTART; //信号在执行某些可中断的系统调用时触发，系统调用不会返回错误码，而是会在信号处理完毕后，重新启动
    sigfillset(&act.sa_mask); //设置信号掩码，使所有信号在信号处理期间被屏蔽，这样可以避免信号处理期间被其他信号打断，确保信号处理的原子性。
    sigaction(sig, &act, NULL);
}

void unified_event_handler(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    epoll_event evs[1024];
    int epfd = epoll_create(1024);
    addfd(epfd, sock);

    //创建一对互联的套接字
    socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipefd);
    setnonblocking(sig_pipefd[1]); //设为无阻塞
    addfd(epfd, sig_pipefd[0]); //监听可读事件

    add_sig(SIGHUP); //挂起信号
    add_sig(SIGINT); //中断信号
    add_sig(SIGTERM); //终止信号
    add_sig(SIGCHLD); //子进程状态改变信号

    bool stop = false;

    while (!stop) {
        int ret = epoll_wait(epfd, evs, 1024, -1);
        if (ret < 0) {
            printf("epoll_wait error\n");
            break;
        }
        for (int i = 0; i < ret; i++) {
            int fd = evs[i].data.fd;
            if (fd == sock) {
                sockaddr_in client;
                socklen_t client_len = sizeof(client);
                int cfd = accept(sock, (struct sockaddr *) &client, &client_len);
                addfd(epfd, cfd);
            } else if (fd == sig_pipefd[0] && (evs[i].events & EPOLLIN)) {
                //有事件发生，管道可读
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; i++) {
                        switch (signals[i]) {
                            case SIGCHLD:
                            case SIGHUP: {
                                continue;
                            }
                            case SIGTERM:
                            case SIGINT: {
                                stop = true;
                            }
                        }
                    }
                }
            }
        }
    }
    printf("close \n");
    close(sock);
    close(sig_pipefd[1]);
    close(sig_pipefd[0]);
}

//网络编程相关信号
//SIGHUP    当挂起进程的控制终端时，SIGHUP信号将触发，通常利用SIGHUP信号来强制服务器重读配置文件
//SIGPIPE   往一个读端关闭的管道或socket连接中写数据将引发SIGPIPE，需要在代码中捕获并处理，至少忽略，因为SIGPIPE默认行为是结束进程
//SIGURG    通知应用程序带外数据到达
int g_fd = -1;

void sig_urg(int sig) {
    // 可以将connfd初始化为-1，在信号处理函数内判断connfd的值，若为-1，则做个标记，
    // 等到accpet函数返回时，通过判断该标记来处理带外数据。若不为-1，则在信号处理函数内正常处理。
    if (g_fd == -1) {
        return;
    }
    printf("sig_urg \n");
    int pre_errno = errno;
    char buf[1024] = {0};
    int ret = recv(g_fd, buf, sizeof(buf) - 1, MSG_OOB); //带外数据
    printf("get %d bytes oob data %s\n", ret, buf);
    errno = pre_errno;
}

void add_sig(int sig, void (*fn)(int)) {
    struct sigaction act;
    act.sa_handler = fn;
    act.sa_flags |= SA_RESTART;
    sigfillset(&act.sa_mask);
    sigaction(sig, &act, NULL);
}

void urg_server_handler(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    //把信号注册函数，放到accept函数前
    //将SIGURG信号处理函数放在accpet之前注册，进程可能会在三次握手建立完与accpet返回这个时间间隔内收到带外数据，然后触发SIGURG信号，
    //而我们在SIGURG信号中要用到socketfd，但此时accpet函数没有返回，即没有socketfd,这样就会发生错误
    add_sig(SIGURG, sig_urg);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    g_fd = accept(sock, (struct sockaddr *) &client, &client_len);

    //使用SIGURG信号之前，必须设置socket的宿主进程或进程组
    fcntl(g_fd, F_SETOWN, getpid());

    char buf[1024] = {0};
    while (1) {
        memset(buf, 0, sizeof(buf));
        int ret = recv(g_fd, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            break;
        }
        printf("get %d bytes data %s\n", ret, buf);
    }
    close(g_fd);

    close(sock);
}

void urg_client_handler(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    send(sock, "hello", strlen("hello"), 0);
    send(sock, "test", strlen("test"), MSG_OOB);
    send(sock, "world", strlen("world"), 0);

    //防止过早退出，带外数据未能读出
    //sleep(3);

    close(sock);
}

int main(int argc, char *argv[]) {
    //unified_event_handler(atoi(argv[1]));
    if (strcmp(argv[1], "server") == 0) {
        urg_server_handler(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        urg_client_handler(atoi(argv[2]));
    }
    return 0;
}
