#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/mman.h>
using namespace std;

//多进程编程

//fork系统调用
//pid_t fork(void);
//该函数每次调用都返回2次，父进程返回的是子进程的PID，子进程返回的是0，失败返回-1
//子进程代码与父进程完全相同，还会复制父进程的数据，数据的复制采用写时复制，任一进程（父或子）对数据进行了写操作，复制才会发生
//父进程打开的文件描述符在子进程也是打开的，且文件描述符的引用计数加1

//exec系列系统调用
//int execl(const char* path, const char* arg, ...);
//int execlp(const char* file, const char* arg, ...);
//int execle(const char* path, const char* arg, ..., char* const envp[]);
//int execv(const char* path, char* const argv[]);
//int execvp(const char* file, char* const argv[]);
//int execve(const char* path, char* const argv[], char* const envp[]);

//处理僵尸进程
//对于多进程，父进程需要跟踪子进程的退出状态，当子进程结束运行时，内核不会立即释放该进程的进程表表项，以满足父进程后续对该子进程退出信息的查询。
// 子进程结束运行之后，父进程读取其退出状态之前，我们称该子进程处于 僵尸态。
//另一种子进程进入僵尸态的情况：父进程结束或异常终止，而子进程继续运行，此时子进程的PPID被操作系统设置为1，init进程接管子进程，并等待结束。
//在父进程退出之后，子进程退出之前，子进程处理 僵尸态。

//由此可见，如果父进程没有正确处理子进程的返回信息，子进程都将停留在僵尸态

//下面函数在父进程中调用，等待子进程退出，获取子进程的返回信息，避免僵尸进程产生
//pid_t wait(int* stat_loc);
//pid_t waitpid(pid_t pid, int* stat_loc, int options);
//options参数通常用 WNOHANG，非阻塞

//最好在某个子进程退出之后再调用waitpid,父进程通过 SIGCHLD 信号得知子进程退出
void child_exit_handler(int sig) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        //对结束子进程的后续处理
    }
}

//管道
//管道是父进程与子进程间通信的常用手段
//要实现父子进程间的双向数据传输，就必须使用2个管道，socket接口中提供了创建全双工管道的系统调用 socketpair

//信号量
//信号量是一种特殊的变量，只能取自然数值并且只支持2种操作，等待(wait) 和 信号(signal)
//信号量的2种操作更常用的称呼是 P，V 操作， P（传递） V（释放）
//P，如果值大于0，就减1，如果值为0，则挂起进程
//V，如果有其他进程等待而挂起，则唤醒，如果没有，则将值加1

//int semget(key_t key, int num_sems, int sem_flags);
//int semop(int sem_id, struct sembuf* sem_ops, size_t num_sem_ops);
//int semctl(int sem_id, int sem_num, int command, ...);

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};

//op为-1时，执行P操作，
//op为1时，执行V操作
void pv(int sem_id, int op) {
    struct sembuf semb;
    semb.sem_num = 0;
    semb.sem_op = op;
    semb.sem_flg = SEM_UNDO; //当进程异常退出时（比如崩溃），系统会尝试恢复这个信号量到它原来的状态，以避免死锁或其他并发问题的发生。
    semop(sem_id, &semb, 1);
}

void sem_test() {
    //创建信号量
    int sem_id = semget(IPC_PRIVATE, 1, 0666);

    union semun un;
    un.val = 1;
    semctl(sem_id, 0, SETVAL, un);

    pid_t pid = fork();
    if (pid < 0) {
        return;
    } else if (pid == 0) {
        printf("子进程\n");

        pv(sem_id, -1);
        printf("子进程获取sem\n");
        sleep(5);
        pv(sem_id, 1);
        exit(0);
    } else {
        printf("父进程\n");

        pv(sem_id, -1);
        printf("父进程获取sem\n");
        sleep(5);
        pv(sem_id, 1);
    }

    waitpid(pid, NULL, 0);
    semctl(sem_id, 0, IPC_RMID); //删除信号量
}

//共享内存
//共享内存是最高效的IPC机制，不涉及进程间的任何数据传输，但会产生竞态
//int shmget(key_t key, size_t size, int shmflg);  创建共享内存
//void* shmat(int shm_id, const void* shm_addr, int shmflg);  将共享内存关联到进程地址空间中
//int shmdt(const void* shm_addr);  从进程地址空间中分离
//int shmctl(int shm_id, int command, struct shmid_ds* buf);  控制共享内存的属性

//共享内存 POSIX 方法
//int shm_open(const char* name, int oflag, mode_t mode);
//int shm_unlink(const char* name);

struct client_data {
    sockaddr_in addr; //地址
    int sock; //文件描述符
    pid_t pid; //子进程PID
    int pipefd[2]; //和父进程通信
};

#define BUFFER_SIZE 1024
#define USER_LIMIT 5
#define PROCESS_LIMIT 4194304

const char *shm_name = "/my_shm";
int sig_pipe[2];
client_data *users = NULL; //所有用户数据
int *sub_map_idx = NULL; //子进程与客户连接索引的映射关系表
int user_cnt = 0; //用户数量
int epoll_fd = 0; //epoll描述符
int listen_fd = 0; //监听描述符
int child_stop = false; //子进程退出
int shm_fd; //共享内存描述符
char *share_mem = NULL; //共享内存

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

void sig_handler(int sig) {
    int pre_errno = errno;
    int msg = sig;
    send(sig_pipe[1], &msg, 1, 0);
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

void del_resource() {
    close(sig_pipe[1]);
    close(sig_pipe[0]);
    close(listen_fd);
    close(epoll_fd);

    shm_unlink(shm_name);

    delete[] users;
    delete[] sub_map_idx;
}

void child_stop_handler(int sig) {
    child_stop = true;
}

int child_run(int idx, client_data *users, char *share_mem) {
    epoll_event evs[1024];
    int child_epfd = epoll_create(1024);

    int pid = users[idx].pid;
    int sock = users[idx].sock;
    addfd(child_epfd, sock); //监听客户连接

    int pipefd = users[idx].pipefd[1]; //父进程使用pipefd[0]读写，子进程使用pipefd[1]读写
    addfd(child_epfd, pipefd); //监听父进程通信管道

    add_sig(SIGTERM, child_stop_handler, false);

    while (!child_stop) {
        int ret = epoll_wait(child_epfd, evs, 1024, -1);
        if (ret < 0 && errno != EINTR) {
            printf("epoll_wait failed\n");
            break;
        }
        for (int i = 0; i < ret; i++) {
            int fd = evs[i].data.fd;
            //子进程负责的连接有数据到达
            if ((fd == sock) && (evs[i].events & EPOLLIN)) {
                memset(share_mem + idx * BUFFER_SIZE, 0, BUFFER_SIZE);
                ret = recv(fd, share_mem + idx * BUFFER_SIZE, BUFFER_SIZE - 1, 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        child_stop = true;
                    }
                } else if (ret == 0) {
                    child_stop = true;
                } else {
                    //读取客户数据后，通知主进程来处理
                    send(pipefd, &idx, sizeof(idx), 0);
                    printf("pid %d client %d recv data %s\n", pid, fd, share_mem + idx * BUFFER_SIZE);
                }
            } else if ((fd == pipefd) && (evs[i].events & EPOLLIN)) {
                //主进程通知子进程，
                int client = 0;
                ret = recv(fd, &client, sizeof(client), 0);
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        child_stop = true;
                    }
                } else if (ret == 0) {
                    child_stop = true;
                } else {
                    send(sock, share_mem + client * BUFFER_SIZE, BUFFER_SIZE, 0);
                }
            }
        }
    }

    close(sock);
    close(pipefd);
    close(child_epfd);

    return 0;
}

void multi_process_test(int port) {
    printf("%d\n", port);
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(listen_fd, (struct sockaddr *) &addr, sizeof(addr));
    listen(listen_fd, 5);

    users = new client_data[USER_LIMIT + 1];
    sub_map_idx = new int[PROCESS_LIMIT];
    if (!users) {
        printf("users init error\n");
        return;
    }
    if (!sub_map_idx) {
        printf("sub_map_idx init error\n");
        return;
    }
    for (int i = 0; i < PROCESS_LIMIT; i++) {
        sub_map_idx[i] = -1;
    }

    epoll_event evs[1024];
    epoll_fd = epoll_create(1024);
    addfd(epoll_fd, listen_fd);

    socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipe);
    setnonblocking(sig_pipe[1]);
    addfd(epoll_fd, sig_pipe[0]);

    add_sig(SIGCHLD, sig_handler);
    add_sig(SIGTERM, sig_handler);
    add_sig(SIGINT, sig_handler);
    add_sig(SIGPIPE, SIG_IGN);

    bool server_stop = false;
    bool terminate = false;

    //创建共享内存
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, USER_LIMIT * BUFFER_SIZE);
    share_mem = (char *) mmap(NULL, USER_LIMIT * BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (!share_mem) {
        printf("mmap failed\n");
        return;
    }
    close(shm_fd);

    while (!server_stop) {
        int ret = epoll_wait(epoll_fd, evs, 1024, -1);
        if ((ret < 0) && errno != EINTR) {
            printf("epoll_wait failed\n");
            break;
        }
        for (int i = 0; i < ret; i++) {
            int fd = evs[i].data.fd;
            if (fd == listen_fd) {
                sockaddr_in client;
                socklen_t client_len = sizeof(client);
                int cfd = accept(listen_fd, (struct sockaddr *) &client, &client_len);
                if (cfd < 0) {
                    printf("accept failed\n");
                    continue;
                }
                if (user_cnt >= USER_LIMIT) {
                    send(cfd, "too many users\n", strlen("too many users\n"), 0);
                    close(cfd);
                    continue;
                }
                //保存用户数据
                users[user_cnt].sock = cfd;
                users[user_cnt].addr = client;
                //在主进程和子进程间创建管道，传递必要的数据
                socketpair(AF_UNIX, SOCK_STREAM, 0, users[user_cnt].pipefd);

                pid_t pid = fork();
                if (pid < 0) {
                    close(cfd);
                    continue;
                } else if (pid == 0) {
                    //子进程
                    close(epoll_fd);
                    close(listen_fd);
                    close(users[user_cnt].pipefd[0]); //子进程通过pipefd[1]通信，不需要pipefd[0]，关闭
                    close(sig_pipe[0]);
                    close(sig_pipe[1]);

                    child_run(user_cnt, users, share_mem);

                    //释放共享内存
                    munmap(share_mem, USER_LIMIT * BUFFER_SIZE);

                    exit(0);
                } else {
                    //父进程中
                    close(cfd);
                    close(users[user_cnt].pipefd[1]); //父进程通过pipefd[0]通信，不需要pipefd[1]，半闭

                    addfd(epoll_fd, users[user_cnt].pipefd[0]); //监听读管道

                    users[user_cnt].pid = pid;

                    //printf("%d\n", pid);
                    //printf("%d\n", user_cnt);

                    sub_map_idx[pid] = user_cnt; //建立进程PID与用户数据索引的映射

                    user_cnt++;
                }
            } else if ((fd == sig_pipe[0]) && (evs[i].events & EPOLLIN)) {
                //处理信号事件
                int sig;
                char signals[1024];
                ret = recv(fd, signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; i++) {
                        switch (signals[i]) {
                            case SIGCHLD: {
                                //子进程退出
                                pid_t pid;
                                int stat;
                                while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                                    int del_idx = sub_map_idx[pid];
                                    sub_map_idx[pid] = -1;
                                    if (del_idx < 0 || del_idx > USER_LIMIT) {
                                        continue;
                                    }
                                    //删除管道的监听
                                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, users[del_idx].pipefd[0], 0);
                                    close(users[del_idx].pipefd[0]);

                                    users[del_idx] = users[--user_cnt];
                                    sub_map_idx[users[del_idx].pid] = del_idx;

                                    printf("child %d exit, now we have %d users\n", del_idx, user_cnt);
                                }
                                if (terminate && user_cnt == 0) {
                                    server_stop = true;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT: {
                                //服务器结束
                                if (user_cnt == 0) {
                                    server_stop = true;
                                    break;
                                }
                                //遍历子进程，并杀死
                                for (int i = 0; i < user_cnt; i++) {
                                    int pid = users[i].pid;
                                    kill(pid, SIGTERM);
                                }
                                terminate = true;
                                break;
                            }
                        }
                    }
                }
            } else if (evs[i].events & EPOLLIN) {
                //如果某个子进程向父进程写入数据
                int child = 0;
                ret = recv(fd, &child, sizeof(child), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    //向除负责child客户连接的子进程外的其他子进程发送消息，通知它们有客户数据要写
                    for (int j = 0; j < user_cnt; j++) {
                        if (users[j].pipefd[0] != fd) {
                            send(users[j].pipefd[0], &child, sizeof(child), 0);
                        }
                    }
                }
            }
        }
    }
    del_resource();
}

//消息队列
//消息队列是在两个进程之间传递二进制块数据的一种简单有效的方式，每个数据块都有一个特定的类型，接收方可以根据类型来有选择地接收数据。
//int msgget(key_t key, int msgflg);
//int msgsnd(int msqid, const void* msg_ptr, size_t msg_sz, int msgflg);  发送消息
//int msgrcv(int msqid, void* msg_ptr, size_t msg_sz, long int msgtype, int msgflg); 接收消息
//int msgctl(int msqid, int command, struct msqid_ds* buf);

//IPC命令
//ipcs

//在进程间传递文件描述符
//fork调用后，父进程打开的文件描述符在子进程中仍然保持打开，文件描述符可以很方便从父进程传递到子进程，但传递文件描述符并不是传递一个文件描述符的值
//而是要接收进程创建一个新的文件描述符，该文件描述符和发送进程中被传递的文件描述符指向内核中相同的文件表项。

//sock是用来传递消息的socket，file_fd是要传递的文件描述符

static const int CONTROL_LEN = CMSG_LEN(sizeof(int));

void send_fd(int sock, int file_fd) {
    iovec iov[1]; //iov 与 msg 的顺序不能变，可以发送错误
    msghdr msg;

    char buf[0];
    iov[0].iov_base = buf;
    iov[0].iov_len = 1;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    // 如果需要发送文件描述符或其他控制信息
    cmsghdr cm;
    cm.cmsg_len = CONTROL_LEN;
    cm.cmsg_level = SOL_SOCKET;
    cm.cmsg_type = SCM_RIGHTS;
    *(int *) CMSG_DATA(&cm) = file_fd;

    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    //printf("send file_fd %d\n", file_fd);

    sendmsg(sock, &msg, 0);
}

int recv_fd(int sock) {
    iovec iov[1]; //iov 与 msg 的顺序不能变，可以发送错误
    msghdr msg;

    char buf[0];
    iov[0].iov_base = buf;
    iov[0].iov_len = 1;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;

    cmsghdr cm;
    msg.msg_control = &cm;
    msg.msg_controllen = CONTROL_LEN;

    recvmsg(sock, &msg, 0);

    int file_fd = *(int *) CMSG_DATA(&cm);

    //printf("get file_fd %d\n", file_fd);

    return file_fd;
}

void parent_child_fd_test() {
    int pipefd[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd);

    int file_fd;

    pid_t pid = fork();
    if (pid < 0) {
        return;
    } else if (pid == 0) {
        //子进程
        close(pipefd[0]);
        file_fd = open("test.txt", O_RDWR, 0666);
        send_fd(pipefd[1], file_fd);
        close(file_fd);
        exit(0);
    } else {
        //父进程
        close(pipefd[1]);
        file_fd = recv_fd(pipefd[0]);
        char buf[1024] = {0};
        read(file_fd, buf, sizeof(buf));
        printf("fd %d get data %s\n", file_fd, buf);
        close(file_fd);
    }
}

int main(int argc, char *argv[]) {
    //sem_test();
    //multi_process_test(atoi(argv[1]));
    parent_child_fd_test();
    return 0;
}
