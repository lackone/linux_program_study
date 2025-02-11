#ifndef PROCESS_POOL_H
#define PROCESS_POOL_H
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <sys/wait.h>

int sig_pipefd[2]; //用于处理信号的管道，以实现统一事件源

int setnonblocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void add_fd(int epfd, int fd) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

void remove_fd(int epfd, int fd) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void sig_handler(int sig) {
    int pre_errno = errno;
    int msg = sig;
    send(sig_pipefd[1], &msg, 1, 0);
    errno = pre_errno;
}

void add_sig(int sig, void (*fn)(int), bool restart = true) {
    struct sigaction sa;
    sa.sa_handler = fn;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

class sub_process {
public:
    sub_process() : _pid(-1) {
    }

public:
    pid_t _pid; //子进程PID
    int _pipefd[2]; //用于与父进程通信
};

template<typename T>
class process_pool {
private:
    //构造函数私有，只能通过静态方法instance()创建实例
    process_pool(int listen_fd, int process_num = 8) : _listen_fd(listen_fd),
                                                       _process_num(process_num),
                                                       _idx(-1),
                                                       _stop(false) {
        _sub_process = new sub_process[_process_num];

        //创建_process_num个子进程，并建立与父进程之间的管道
        for (int i = 0; i < _process_num; i++) {
            //创建管道
            socketpair(AF_UNIX, SOCK_STREAM, 0, _sub_process[i]._pipefd);

            //创建子进程
            _sub_process[i]._pid = fork();

            //fork会返回2次，在父进程里continue，子进程直接break
            if (_sub_process[i]._pid > 0) {
                //父进程
                close(_sub_process[i]._pipefd[1]);
                continue;
            } else {
                //子进程
                close(_sub_process[i]._pipefd[0]);
                _idx = i;
                break;
            }
        }
    }

public:
    //单例模式，保证最多创建一个process_pool实例
    static process_pool<T> *instance(int listen_fd, int process_num = 8) {
        if (!_instance) {
            _instance = new process_pool<T>(listen_fd, process_num);
        }
        return _instance;
    }

    ~process_pool() {
        delete[] _sub_process;
    }

    //启动进程池
    void run() {
        //父进程_idx值为-1，子进程值大于等于0
        //通过此判断，运行父进程代码，还是子进程代码
        if (_idx != -1) {
            run_child();
            return;
        }
        run_parent();
    }

private:
    //统一事件源
    void setup_sig_pipe() {
        _epfd = epoll_create(5);

        socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipefd);
        setnonblocking(sig_pipefd[1]);
        add_fd(_epfd, sig_pipefd[0]); //监听管道读

        //设置信号处理
        add_sig(SIGCHLD, sig_handler);
        add_sig(SIGTERM, sig_handler);
        add_sig(SIGINT, sig_handler);
        //当一个进程试图向一个已关闭的管道或socket连接的一端写入数据时，操作系统会向该进程发送SIGPIPE信号，默认情况下这个信号会导致进程终止。
        //直接忽略该信号，防止程序在这种情况下被意外终止。
        add_sig(SIGPIPE, SIG_IGN);
    }

    void run_child() {
        setup_sig_pipe();

        //每个子进程通过idx找到与父进程通信的管道
        int pipefd = _sub_process[_idx]._pipefd[1];

        add_fd(_epfd, pipefd);

        epoll_event evs[MAX_EVENT_NUM];
        T *users = new T[USER_PER_PROCESS];

        int num = 0;
        int ret = 0;

        while (!_stop) {
            num = epoll_wait(_epfd, evs, MAX_EVENT_NUM, -1);
            if (num < 0 && errno != EINTR) {
                printf("epoll_wait error\n");
                break;
            }
            for (int i = 0; i < num; i++) {
                int fd = evs[i].data.fd;
                if ((fd == pipefd) && (evs[i].events & EPOLLIN)) {
                    //读取从父进程传递过来的数据
                    int client = 0;
                    ret = recv(fd, &client, sizeof(client), 0);
                    if (((ret < 0) && (errno != EAGAIN)) || ret == 0) {
                        continue;
                    } else {
                        sockaddr_in client;
                        socklen_t client_len = sizeof(client);
                        int cfd = accept(_listen_fd, (struct sockaddr *) &client, &client_len);
                        if (cfd < 0) {
                            printf("errno %d\n", errno);
                            continue;
                        }
                        add_fd(_epfd, cfd);
                        //模板类T必须实现init方法，初始化一个客户端
                        users[cfd].init(_epfd, cfd, client);
                    }
                } else if ((fd == sig_pipefd[0]) && (evs[i].events & EPOLLIN)) {
                    //处理子进程接收到的信号
                    int sig;
                    char signals[1024];
                    ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                    if (ret <= 0) {
                        continue;
                    } else {
                        for (int i = 0; i < ret; i++) {
                            switch (signals[i]) {
                                case SIGCHLD: {
                                    pid_t pid;
                                    int stat;
                                    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                        continue;
                                    }
                                    break;
                                }
                                case SIGTERM:
                                case SIGINT: {
                                    _stop = true;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                    }
                } else if (evs[i].events | EPOLLIN) {
                    //其他数据可读，必然是客户端请求到来
                    users[fd].process();
                } else {
                    continue;
                }
            }
        }

        delete[] users;
        users = NULL;
        close(pipefd);
        //close(_listen_fd); //由哪个函数创建就应该由哪个函数销毁
        close(_epfd);
    }

    void run_parent() {
        setup_sig_pipe();

        //父进程监听_listen_fd
        add_fd(_epfd, _listen_fd);

        epoll_event evs[MAX_EVENT_NUM];

        int num = 0;
        int ret = 0;
        int new_conn = 1; //有新连接到来
        int sub_process_cnt = 0; //保存每次轮询的下标

        while (!_stop) {
            num = epoll_wait(_epfd, evs, MAX_EVENT_NUM, -1);
            if (num < 0 && errno != EINTR) {
                printf("epoll_wait error\n");
                break;
            }
            for (int i = 0; i < num; i++) {
                int fd = evs[i].data.fd;
                if (fd == _listen_fd) {
                    //如果有新的连接到来，采用Round Robin方法将其分配给一个子进程处理
                    int ix = sub_process_cnt;
                    do {
                        if (_sub_process[ix]._pid != -1) {
                            break;
                        }
                        ix = (ix + 1) % _process_num;
                    } while (ix != sub_process_cnt);

                    if (_sub_process[ix]._pid == -1) {
                        _stop = true;
                        break;
                    }

                    sub_process_cnt = (ix + 1) % _process_num;

                    //向子进程发送消息，有连接过来
                    send(_sub_process[ix]._pipefd[0], &new_conn, sizeof(new_conn), 0);

                    printf("send request to child %d\n", ix);
                } else if ((fd == sig_pipefd[0]) && (evs[i].events & EPOLLIN)) {
                    int sig;
                    char signals[1024];
                    ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
                    if (ret <= 0) {
                        continue;
                    } else {
                        for (int i = 0; i < ret; i++) {
                            switch (signals[i]) {
                                case SIGCHLD: {
                                    pid_t pid;
                                    int stat;
                                    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                        for (int j = 0; j < _process_num; j++) {
                                            if (_sub_process[j]._pid == pid) {
                                                //如果子进程退出了，父进程关闭相应的通信管道，并设置_pid=-1
                                                printf("child %d exit\n", j);
                                                close(_sub_process[j]._pipefd[0]);
                                                _sub_process[j]._pid = -1;
                                            }
                                        }
                                    }
                                    //如果所有子进程都退出，父进程也退出
                                    _stop = true;
                                    for (int j = 0; j < _process_num; j++) {
                                        if (_sub_process[j]._pid != -1) {
                                            _stop = false;
                                        }
                                    }
                                    break;
                                }
                                case SIGTERM:
                                case SIGINT: {
                                    printf("kill all child \n");
                                    for (int j = 0; j < _process_num; j++) {
                                        int pid = _sub_process[j]._pid;
                                        if (pid != -1) {
                                            kill(pid, SIGTERM);
                                        }
                                    }
                                    break;
                                }
                                default: {
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    continue;
                }
            }
        }

        //close(_listen_fd); //由创建关闭这个描述符
        close(_epfd);
    }

private:
    static const int MAX_PROCESS_NUM = 16; //最大子进程数量
    static const int USER_PER_PROCESS = 65536; //子进程最多处理的客户数量
    static const int MAX_EVENT_NUM = 10000; //epoll最多能处理的事件数
    int _process_num; //进程数
    int _idx; //子进程在池中的序号
    int _epfd; //每个进程都有一个epoll
    int _listen_fd; //监听socket
    int _stop; //是否停止运行
    sub_process *_sub_process; //保存所有子进程的描述信息
    static process_pool<T> *_instance; //进程池静态实例
};

template<typename T>
process_pool<T> *process_pool<T>::_instance = NULL;

#endif //PROCESS_POOL_H
