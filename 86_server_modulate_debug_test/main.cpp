#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

using namespace std;

//最大文件描述符

//ulimit -n

//临时的，只在当前session中有效
//ulimit -SHn 65535

// /etc/security/limits.conf 加入如下2项：
// * hard nofile 65535
// * soft nofile 65535
//第一行是系统的硬限制，第二行是软限制

//修改系统级文件描述符数限制
// sysctl -w fs.file-max=65535
//也是临时更改系统限制

// /etc/sysctl.conf 添加如下一项
// fs.file-max=65535

//执行 sysctl -p 命令

//调整内核参数
// /proc/sys/fs 目录下的部分文件
// /proc/sys/fs/file-max   系统级文件描述符数限制，修改后需要把 /proc/sys/fs/inode-max设置为file-max值的3-4倍
// /proc/sys/fs/epoll/max_user_watches   用户能往epoll内核事件中注册的事件总量

// /proc/sys/net 目录下的部分文件
// /proc/sys/net/core/somaxconn  指定listen监听队列里，能够建立完整连接从而进入ESTABLISHED状态的socket最大数目
// /proc/sys/net/ipv4/tcp_max_syn_backlog  指定listen监听队列里，能够转移至ESTABLISHED或者SYN_RCVD状态的socket最大数目
// /proc/sys/net/ipv4/tcp_wmem   TCP写缓冲区的最小值   默认值   最大值
// /proc/sys/net/ipv4/tcp_rmem   TCP读缓冲区的最小值   默认值   最大值
// /proc/sys/net/ipv4/tcp_syncookies  是否打开TCP同步标签，


// 用gdb调试多进程程序

// attach PID  附加进程
// set follow-fork-mode [parent|child]   在执行fork调用后是调试父进程还是子进程

// 用gdb调试多线程程序
// info threads  显示可调试所有线程
// thread ID  调试目标ID指定的线程
// set scheduler-locking [off|on|step]

static const char *request =
        "GET / HTTP/1.1\r\nConnection: keep-alive\r\n\r\nxxxxxxxxxxxxxxxxxxx";

int setnonblocking(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void addfd(int epfd, int fd) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLOUT | EPOLLET | EPOLLERR;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking(fd);
}

//向服务器写入len字节的数据
bool write_bytes(int fd, const char *buf, int len) {
    int bytes_write = 0;
    printf("write out %d bytes to socket %d\n", len, fd);
    while (1) {
        bytes_write = send(fd, buf, len, 0);
        if (bytes_write == -1) {
            return false;
        } else if (bytes_write == 0) {
            return false;
        } else {
            len -= bytes_write;
            buf = buf + bytes_write;
            if (len <= 0) {
                return true;
            }
        }
    }
}

//读取数据
bool read_once(int fd, char *buf, int len) {
    int bytes_read = 0;
    memset(buf, 0, len);
    bytes_read = recv(fd, buf, len, 0);
    if (bytes_read == -1) {
        return false;
    } else if (bytes_read == 0) {
        return false;
    }
    printf("read in %d bytes from socket %d with content %s\n", bytes_read, fd, buf);
    return true;
}

//向服务器发起num个TCP连接
void start_conn(int epfd, int num, const char *ip, int port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    for (int i = 0; i < num; i++) {
        sleep(1);
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            continue;
        }
        if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == 0) {
            printf("build connection %d\n", i);
            addfd(epfd, fd);
        }
    }
}

void close_conn(int epfd, int fd) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void connect_test(const char *ip, int port, int num) {
    int epfd = epoll_create(100);
    start_conn(epfd, num, ip, port);
    epoll_event evs[10000];
    char buf[2048] = {0};
    while (1) {
        int ret = epoll_wait(epfd, evs, 10000, -1);
        for (int i = 0; i < ret; i++) {
            int fd = evs[i].data.fd;
            if (evs[i].events & EPOLLIN) {
                if (!read_once(fd, buf, 2048)) {
                    close_conn(epfd, fd);
                }
                epoll_event ev;
                ev.events = EPOLLOUT | EPOLLET | EPOLLERR;
                ev.data.fd = fd;
                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
            } else if (evs[i].events & EPOLLOUT) {
                if (!write_bytes(fd, request, strlen(request))) {
                    close_conn(epfd, fd);
                }
                epoll_event ev;
                ev.events = EPOLLIN | EPOLLET | EPOLLERR;
                ev.data.fd = fd;
                epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
            } else if (evs[i].events & EPOLLERR) {
                close_conn(epfd, fd);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    connect_test(argv[1], atoi(argv[2]), atoi(argv[3]));
    return 0;
}
