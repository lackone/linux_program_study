#include <csignal>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
using namespace std;

//无名管道
//管道可以把一个程序的输出直接连接到另一个程序的输入，以此来建立连接。 管道分为两种，一种 是无名管道，一种是有名管道。
//管道的本质是在内核空间上的一段特殊内存区域。
//无名管道是被创建在内核空间上的。无名管道使用时有固定的读端和写端，发送消息需要向管道的写端写入，接收消息需要向管道的读端读取，这样即可完成数据的传递了。
//int pipe(int pipefd[2]);
//pipefd[0]视为管道的读端，而 pipefd[1]视为管道的写端

//（1）无名管道只能用于具有亲缘关系的进程之间通信(如父子进程)。
//（2）类似于单工的模式，无名管道具有固定的读端与写端。
//（3）无名管道虽然是特殊的文件，但对它的读写可以使用文件 I/O 中 read()函数、write()函数直 接操作文件描述符即可。
//（4）无名管道本质是一段内核空间中的内存段，因此不能使用 lseek()函数对管道进行定位操作。
//（5）无名管道的操作属于一次性操作，一旦对管道中的数据进行读取，读取的数据将会从管道 中移除。
//（6）无名管道的大小是固定的，向无名管道写入数据，当管道写满时，继续写入将会阻塞
void pipe_size_test() {
    int fd[2];

    pipe(fd);

    char buf[1024] = {0};
    int cnt = 0;
    while (1) {
        int n = write(fd[1], buf, sizeof(buf));
        cnt += n;
        //无名管道的大小为 65536 字节（64KB），当管道被写满后，写操作将会被阻塞。
        printf("cnt=%d\n", cnt);
    }
}

void pipe_size_test2() {
    int fd[2];
    pipe(fd);

    char buf[1024] = {0};
    int cnt = 0;
    while (1) {
        write(fd[1], buf, sizeof(buf));
        cnt++;
        printf("cnt=%d\n", cnt);
        if (cnt == 64) {
            //无名管道中数据被写满后，写操作将会阻塞，当管道中出现 4KB 以上的空闲空间时，可以继续写入 4KB 的整倍数的数据。
            read(fd[0], buf, sizeof(buf));
            read(fd[0], buf, sizeof(buf));
            read(fd[0], buf, sizeof(buf));
            read(fd[0], buf, sizeof(buf));
        }
    }
}

void pipe_size_test3() {
    int fd[2];
    pipe(fd);

    close(fd[0]); //关闭读端

    //当无名管道的读端被关闭时，从写端写入数据，管道将会破裂，进程将会退出。
    char buf[1024] = {0};
    write(fd[1], buf, sizeof(buf));
}

//当管道无数据时，读操作将会阻塞；当管道中有数据时，且写端关闭时，读操作可以读取，不会阻塞。

//无名管道的通信
void pipe_comm_test() {
    int fd[2];
    char buf[32] = {0};
    int n = 0;

    int fd1 = open("test1.txt", O_RDONLY);
    int fd2 = open("test2.txt", O_CREAT | O_RDWR | O_TRUNC, 0664);

    pipe(fd);

    pid_t pid = fork();

    if (pid < 0) {
        return;
    }
    if (pid == 0) {
        //子进程
        //从管道中读取数据，写入 test2.txt
        while ((n = read(fd[0], buf, sizeof(buf))) > 0) {
            write(fd2, buf, n);
        }
    } else {
        //从 test1.txt 中读取数据，写入管道中
        while ((n = read(fd1, buf, sizeof(buf))) > 0) {
            write(fd[1], buf, n);
        }
    }
}

//有名管道
//有名管道 FIFO 与无名管道 pipe 类似，二者最大的区别在于有名管道在文件系统中拥有一个名称，而无名管道则没有。
//可以使用 Shell 命令直接创建有名管道，使用时只需终端输入“mkfifo + 管道名称” 即可，则在当前目录下会生成一个管道文件，其打开方式与普通文件的打开方式一样。

//（1）有名管道可以使两个互不相关的进程进行通信，无名管道则有这方面的局限。
//（2）有名管道可以通过路径名指出，在文件系统中可见，但文件只是一个类似的标记，管道中 的数据实际上在内核内存上，这一点与无名管道一致，因此对于有名管道而言同样不可以使用 lseek()函数定位处理。
//（3）有名管道数据读写遵循先进先出的原则。
//（4）对有名管道的操作与文件一致，采用文件 I/O 的方式。
//（5）默认情况下，如果当前有名管道中无数据，读操作将会阻塞。
//（6）如果有名管道空间已满，写操作会阻塞。
//int mkfifo(const char *pathname, mode_t mode);
void fifo_test() {
    mkfifo("fifo", 0664);

    //只读的方式打开时，打开将会被阻塞
    //打开操作以读写或只写的方式，将不会阻塞。
    //int fd = open("fifo", O_RDONLY);
    int fd = open("fifo", O_RDWR);
    if (fd > 0) {
        printf("fd = %d\n", fd);
    }
    printf("open after\n");
}

void fifo_test2() {
    int fd;
    if (mkfifo("fifo", 0664) < 0) {
        if (errno == EEXIST) {
            //判断管道是否存在
            fd = open("fifo", O_RDWR);
            printf("fd = %d\n", fd);
        } else {
            perror("mkfifo error");
        }
    } else {
        fd = open("fifo", O_RDWR);
        printf("fd = %d\n", fd);
    }
}

void fifo_comm_a() {
    int fd;
    char buf[32] = {0};

    if (mkfifo("fifo", 0664) < 0) {
        if (errno == EEXIST) {
            //判断管道是否存在
            fd = open("fifo", O_RDWR);
            printf("fd = %d\n", fd);
        } else {
            perror("mkfifo error");
        }
    } else {
        fd = open("fifo", O_RDWR);
        printf("fd = %d\n", fd);
    }

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        //写入管道
        write(fd, buf, strlen(buf));

        if (strncmp(buf, "exit", 4) == 0) {
            break;
        }
    }
}

void fifo_comm_b() {
    int fd;
    char buf[32] = {0};

    if (mkfifo("fifo", 0664) < 0) {
        if (errno == EEXIST) {
            //判断管道是否存在
            fd = open("fifo", O_RDWR);
            printf("fd = %d\n", fd);
        } else {
            perror("mkfifo error");
        }
    } else {
        fd = open("fifo", O_RDWR);
        printf("fd = %d\n", fd);
    }

    while (1) {
        int n = read(fd, buf, sizeof(buf)); //从管道读
        buf[n] = '\0';
        if (strncmp(buf, "exit", 4) == 0) {
            system("rm fifo");
            break;
        }
        printf("%s\n", buf);
    }
}

//信号
//信号是进程间通信机制中唯一的异步通信机制，可以将其看成是在软件层次上对中断机制的一种模拟。 一个进程接收信号与处理器接收一个中断请求是很类似的。
//一个进程在接收信号时，通常有三种响应信号的方式。
//（1）忽略信号，即对接收的信号不做任何处理。在 Linux 中，SIGKILL 信号和 SIGSTOP 信号不可以被忽略。
//（2）捕捉信号，即程序可自行定义信号的处理方式（接收信号之后，应该做什么动作），执行 相关的处理函数。
//（3）默认处理，Linux 对大部分信号都已经设置了默认的处理方式。 通俗地说，就是对信号赋 予了自动执行某种操作的能力。

//SIGINT   可以使用物理按键模拟（终端输入 Ctrl+C）  终止进程
//SIGQUIT  与信号 SIGINT 类似，也可以使用物理按键模拟（终端输入 Ctrl+\）  终止进程
//SIGKILL  该信号用来使进程结束，并且不能被阻塞、 处理和忽略  终止进程
//SIGUSR1   用户自定义信号，用户可根据需求自行定义处理方案    无
//SIGUSR2   用户自定义信号，用户可根据需求自行定义处理方案   无
//SIGPIPE   管道破裂，进程收到此信号   终止进程
//SIGALRM   时钟信号，当进程使用定时时钟，时间结束时，收到该信号  终止进程
//SIGCHLD   子进程状态发生改变时，父进程收到此信号  忽略
//SIGSTOP  该信号用于暂停一个进程，且不能被阻塞、处理或忽略  停止一个进程
//SIGTSTP  与 SIGSTOP 类似，可以用物理按键模拟（终端输入 Ctrl+Z）  停止一个进程

//信号的注册
//typedef void (*sighandler_t)(int);
//sighandler_t signal(int signum, sighandler_t handler);
void signal_handler(int signal) {
    if (signal == SIGINT) {
        cout << "SIGINT received" << endl;
    }
}

void signal_test() {
    signal(SIGINT, signal_handler); //注册信号并设置处理方案
    signal(SIGTSTP, SIG_IGN); //忽略此信号

    while (1) {
        cout << "hello" << endl;
        sleep(1);
    }
}

//int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
//参数 signum 用来设置信号的名称（除 SIGKILL 及 SIGSTOP 信号外）；
//参数 oldact 用来保存信号之前的处理方式，
//参数 act 为新设置的处理方式，二者都是指向 sigaction结构体的指针

//信号的发送
//int kill(pid_t pid, int sig);
//（1）当 pid > 0 时，信号发送给进程号为 pid 的进程，即指定进程号发送。
//（2）当 pid== 0 时，信号可以发送给与调用进程在同一进程组的任何一个进程。
//（3）当 pid==-1 时，信号发送给调用进程被允许发送的任何一个进程（除 init 进程外）。
//（4）当 pid<-1 时，信号发送给进程组等于 - pid 下的任何一个进程。
//int raise(int sig);
//raise()函数同样为发送信号，只不过将信号发送给调用进程本身。
void kill_test() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork error");
    }

    if (pid == 0) {
        cout << "子进程" << endl;
        while (1) {
            sleep(1);
        }
    } else {
        cout << "父进程" << endl;
        sleep(3);
        kill(pid, SIGKILL); //杀死子进程
        raise(SIGKILL); //杀死自已
    }
}

//定时器信号
//alarm()函数也称为闹钟函数，它可以在进程中设置一个闹钟，当定时器指定的时间到时，内核就会向进程发送信号 SIGALRM 信号，使进程退出。
//unsigned int alarm(unsigned int seconds);
void signal_handler2(int signal) {
    if (signal == SIGALRM) {
        cout << "SIGALRM received" << endl;
    }
}

void alarm_test() {
    signal(SIGALRM, signal_handler2);

    int ret = alarm(5);

    printf("ret = %d\n", ret);

    //上一次设置闹钟的时间减去经历的时间
    // 5 - 3 = 2
    sleep(3);
    ret = alarm(10);

    printf("ret = %d\n", ret);

    while (1) {
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    //pipe_size_test();
    //pipe_size_test2();
    //pipe_size_test3();
    //pipe_comm_test();
    //fifo_test();
    //fifo_test2();
    //if (strcmp(argv[1], "a") == 0) {
    //    fifo_comm_a();
    //} else if (strcmp(argv[1], "b") == 0) {
    //    fifo_comm_b();
    //}
    //signal_test();
    //kill_test();
    alarm_test();
    return 0;
}
