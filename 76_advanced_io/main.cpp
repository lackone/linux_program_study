#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/sendfile.h>
using namespace std;

//pipe函数
//创建一个管道，实现进程间通信
//int pipe(int fd[2]);
//fd[1]写入数据，fd[0]读取数据

//socketpair,方便创建双向管道
//int socketpair(int domain, int type, int protocol, int fd[2]);

//dup 函数 和 dup2 函数
//dup总是返回系统中最小的可用文件描述符
//int dup(int file_descriptor); //创建新文件描述符，新文件描述符和原有描述符指向相同的文件、管道、网络连接
//int dup2(int file_descriptor, int file_descriptor_two);
void dup_test(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);

    close(STDOUT_FILENO); //关闭标准输出
    dup(cfd); //dup总是返回系统中最小的可用文件描述符，所以返回是1
    printf("hello world\n"); //这里的输出会直接发送到客户端

    close(cfd);
    close(sock);
}

//readv函数 和 writev函数
//readv将数据从文件描述符读到分散的内存块中，writev将多块分散的内存数据一并写入文件描述符中
//ssize_t readv(int fd, const struct iovec* vector, int count);
//ssize_t writev(int fd, const struct iovec* vector, int count);
void readv_writev_test(int port, const char *file_name) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);

    //文件是否有效
    bool valid = true;
    //文件属性
    struct stat file_stat;
    //文件内容
    char *file_buf;

    if (stat(file_name, &file_stat) < 0) {
        valid = false;
    } else {
        if (S_ISDIR(file_stat.st_mode)) {
            valid = false;
        } else if (file_stat.st_mode & S_IROTH) {
            //其他用户的读权限
            int fd = open(file_name, O_RDONLY);
            file_buf = new char[file_stat.st_size + 1];
            memset(file_buf, 0, file_stat.st_size + 1);
            read(fd, file_buf, file_stat.st_size);
        } else {
            valid = false;
        }
    }

    //head_buf使用了多少字节
    int len = 0;
    char head_buf[1024] = {0};
    //如果文件有效，则发送正常的http应答
    if (valid) {
        int ret = snprintf(head_buf, sizeof(head_buf), "%s %s\r\n", "HTTP/1.1", "200 OK");
        len += ret;
        ret = snprintf(head_buf + len, sizeof(head_buf) - len, "Content-Length: %d\r\n", file_stat.st_size);
        len += ret;
        ret = snprintf(head_buf + len, sizeof(head_buf) - len, "%s", "\r\n");

        //使用writev将 head_buf 和 file_buf 一并写出
        struct iovec iov[2];
        iov[0].iov_base = head_buf;
        iov[0].iov_len = strlen(head_buf);
        iov[1].iov_base = file_buf;
        iov[1].iov_len = file_stat.st_size;

        writev(cfd, iov, 2);
    } else {
        //如果文件无效，则
        int ret = snprintf(head_buf, sizeof(head_buf), "%s %s\r\n", "HTTP/1.1", "500 Internal Server Error");
        len += ret;
        ret = snprintf(head_buf + len, sizeof(head_buf) - len, "%s", "\r\n");

        send(cfd, head_buf, strlen(head_buf), 0);
    }
    delete[] file_buf;

    close(cfd);

    close(sock);
}

//sendfile函数
//sendfile函数在2个文件描述符之间直接传递数据，避免了内核缓冲区和用户缓冲区之间的数据拷贝，效率很高。
//in_fd必须是一个支持mmap函数的文件描述符，必须指向真实的文件，不能是socket和管道
//out_fd必须是一个socket
//ssize_t sendfile(int out_fd, int in_fd, off_t* offset, size_t count);
void sendfile_test(int port, const char *file_name) {
    int file_fd = open(file_name, O_RDONLY);
    struct stat file_stat;
    stat(file_name, &file_stat);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);

    sendfile(cfd, file_fd, NULL, file_stat.st_size);
    close(cfd);

    close(sock);
}

//mmap 函数 和 munmap 函数
//mmap用于申请一段内存，将这段内存作为进程间通信的共享内存，也可以将文件直接映射到其中。
//munmap 释放由 mmap创建的这段内存
//void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset);
//int munmap(void* start, size_t length);
//prot设置内存段的访问权限，PROT_READ, PROT_WRITE, PROT_EXEC, PROT_NONE

//splice函数
//splice用于在2个文件描述符之间移动数据，也是零拷贝操作。
//ssize_t splice(int fd_in, loff_t* off_in, int fd_out, loff_t* off_out, size_t len, unsigned int flags);
//使用splice时，fd_in和fd_out必须至少有一个是管道文件描述符
void splice_test(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int cfd = accept(sock, (struct sockaddr *) &client, &client_len);

    int pipefd[2];
    pipe(pipefd);

    //将cfd读取的数据定向到管道中
    splice(cfd, NULL, pipefd[1], NULL, 655535, SPLICE_F_MORE | SPLICE_F_MOVE);
    //再将管道中的数据，定向到cfd中
    splice(pipefd[0], NULL, cfd, NULL, 655535, SPLICE_F_MORE | SPLICE_F_MOVE);

    close(cfd);

    close(sock);
}

//tee函数
//tee在2个管道文件描述符之间复制数据，也是零拷贝操作。
//ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags);
//fd_in 和 fd_out 必须都是管道文件描述符
void tee_test(const char *file_name) {
    int file_fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);

    int pipefd_std[2];
    pipe(pipefd_std);

    int pipefd_file[2];
    pipe(pipefd_file);

    //将标准输入，定向到pipefd_std写入
    splice(STDIN_FILENO, NULL, pipefd_std[1], NULL, 655535, SPLICE_F_MORE | SPLICE_F_MOVE);

    //复制pipefd_std输出，到pipefd_file写入
    tee(pipefd_std[0], pipefd_file[1], 655535, SPLICE_F_NONBLOCK);

    //将pipefd_file输出定向到文件file_fd
    splice(pipefd_file[0], NULL, file_fd, NULL, 655535, SPLICE_F_MORE | SPLICE_F_MOVE);

    //将pipefd_std输出，定向到标准输出
    splice(pipefd_std[0], NULL, STDOUT_FILENO, NULL, 655535, SPLICE_F_MORE | SPLICE_F_MOVE);

    close(file_fd);

    close(pipefd_std[0]);
    close(pipefd_std[1]);
    close(pipefd_file[0]);
    close(pipefd_file[1]);
}

//fcntl函数
//提供了对文件描述符的各种控制操作
int setnonblocking(int fd) {
    int old_fl = fcntl(fd, F_GETFL);
    int new_fl = old_fl | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_fl);
    return old_fl;
}

int main(int argc, char *argv[]) {
    //dup_test(atoi(argv[1]));
    //readv_writev_test(atoi(argv[1]), argv[2]);
    tee_test(argv[1]);
    return 0;
}
