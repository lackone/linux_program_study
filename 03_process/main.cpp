#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
using namespace std;

//进程的基本概念
//进程是指一个具有独立功能的程序在某个数据集合上的一次动态执行过程，它是操作系统进行资源分配和调度的基本单元。

//程序
//（1）二进制格式标识：每个程序文件都包含用于描述可执行文件格式的元信息。
//内核利用此信息来解释文件中的其他信息。现在，大多数 Linux 采用可执行连接格式（Executable and LinkableFormat，ELF）。
//（2）机器语言指令：对程序进行编码。
//（3）程序入口地址：标识程序开始执行时的起始指令位置。
//（4）数据：程序文件包含的变量初始值和程序使用的字面常量。
//（5）符号表及重定位表：描述程序中函数和变量的位置及名称。 这些表有多种用途，其中包括调试和运行时的符号解析（动态链接）。
//（6）共享库和动态链接信息：程序文件所包含的一些字段，列出了程序运行时需要使用的共享库，以及加载共享库的动态链接器的路径名。
//（7）其他信息：程序文件还包含许多其他信息，用以描述如何创建进程。

//进程是程序动态执行的过程，具有并发性、动态性、交互性和独立性等主要特性。
//（1）并发性是指系统中多个进程可以同时并发执行，相互之间不受干扰。
//（2）动态性是指进程都有完整的生命周期，而且在进程的生命周期内，进程的状态是不断变化的，而且进程具有动态的地址空间（包括代码、 数据和进程控制块等）。
//（3）交互性是指进程在执行过程中可能会与其他进程发生直接和间接的通信，如进程同步和进程互斥等，需要为此添加一定的进程处理机制。
//（4）独立性是指进程是一个相对完整的资源分配和调度的基本单位，各个进程的地址空间是相互独立的，因此需要引入一些通信机制才能实现进程之间的通信。

//进程的类型
//（1）交互式进程。
//（2）批处理进程。
//（3）守护进程。

//进程的状态
//（1）运行态（TASK_RUNNING）
//（2）可中断的睡眠态（TASK_INTERRUPTIBLE）
//（3）不可中断的睡眠态（TASK_UNINTERRUPTIBLE）
//（4）停止态（TASK_STOPPED）
//（5）僵尸态（EXIT_ZOMBIE）
//（6）消亡态（EXIT_DEAD）

//进程标识符
//Linux 内核通过唯一的进程标识符（进程的身份证号）PID（Process ID）来标识每个进程。

//进程组与会话组
// PID 为进程 ID，PPID 为父进程（该进程的父亲进程）的 ID，PGID（Process Group ID）为进程组 ID，SID（Session ID）为会话组 ID。

//1．进程组
//每个进程都有一个用数字表示的进程组 ID，表示该进程所属的进程组。
//pid_t getpgid(pid_t pid);
//pid_t getpgrp(void); /* POSIX.1 version */
//pid_t getpgrp(pid_t pid); /* BSD version */
//int setpgid(pid_t pid, pid_t pgid);
//int setpgrp(void); /* System V version */
//int setpgrp(pid_t pid, pid_t pgid); /* BSD version */

//2．会话组
//pid_t getsid(pid_t pid);
//pid_t setsid(void);

//进程的优先级
//进程的特性 nice 值允许进程间接地影响内核的调度算法。
//int getpriority(int which, int who);
//int setpriority(int which, int who, int prio);

//进程的虚拟内存
//当进程创建的时候（程序运行时），系统会为每一个进程分配大小为 4GB 的虚拟内存空间，用于存储进程属性信息。

//虚拟内存管理
//虚拟内存的规划之一是将每个程序使用的内存切割成小型的、固定大小的“页”单元（一般页面的大小为4096字节）。相应地，将 RAM 划分成一系列与虚拟页尺寸相同的页帧。
//内核需要为每一个进程维护一张页映射表。该页映射表中的每个条目指出一个虚拟“页”在 RAM中的所在位置，在进程虚拟地址空间中，并非所有的地址范围都需要页表条目。

//进程的内存布局
//（1） 程序代码段：具有只读属性，包含程序代码（.init 和.text）和只读数据（.rodata）。
//（2）数据段：存放的是全局变量和静态变量。 其中初始化数据段（.data）存放显示初始化的全 局变量和静态变量；未初始化数据段，此段通常也被称为 BSS 段（.bss），存放未进行显示初始化的全局变量和静态变量。
//（3）栈：由系统自动分配释放，存放函数的参数值、局部变量的值、返回地址等。
//（4）堆：存放动态分配的数据，一般由程序动态分配和释放，若程序不释放，程序结束时可能由操作系统回收。 例如，使用 malloc()函数申请空间。
//（5）共享库的内存映射区域：这是 Linux 动态链接器和其他共享库代码的映射区域。

//进程的创建
//fork()函数用于在已有的进程中再创建一个新的进程。这个被创建的新进程被视为子进程，而调用进程成为其父进程。
//fork()函数如果执行失败，父进程得到返回值为-1；如果执行成功，在子进程中得到一个值为0，在父进程中得到一个值为子进程的 ID（一定大于0的整数）。
//一定注意的是，fork()函数不是在父进程中得到两个返回值，而是父子进程分别得到 fork()函数返回的一个值。

//getpid()函数用于获得调用（自身）进程的 ID。
//getppid()函数用于获得调用进程的父进程的 ID。
void fork_test() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("创建进程失败");
    }

    if (pid == 0) {
        cout << "子进程" << endl;
        cout << getpid() << " " << getppid() << endl;
    } else {
        cout << "父进程" << endl;
        cout << getpid() << " " << getppid() << endl;
    }
}

//exec 函数族
//extern char **environ;
//int execl(const char *path, const char *arg, ...);
//int execlp(const char *file, const char *arg, ...);
//int execle(const char *path, const char *arg, ..., char *const envp[]);
//int execv(const char *path, char *const argv[]);
//int execvp(const char *file, char *const argv[]);
//int execvpe(const char *file, char *const argv[],char *const envp[]);
void exec_test() {
    execl("/bin/ls", "ls", "-l", nullptr);

    char *arr[] = {"ls", "-l", nullptr};
    execv("/bin/ls", arr);
}

//vfork()函数
//vfork()函数为调用进程创建一个新的子进程，以便为程序提供尽可能快的 fork()功能。
//pid_t vfork(void);
//子进程在对数据段、堆或栈的任何改变将在父进程恢复执行时为其所见，因此对子进程的操作需要谨慎，不然可能会因为资源共享的问题，造 成对父进程的影响。

//exit()函数和_exit()函数
//exit()函数与_exit()函数最大的区别就在于 exit()函数在终止当前进程之前要检查该进程打开了哪些文件，并把文件缓存区中的内容写回文件
//void exit(int status);
//void _exit(int status);
void exit_test() {
    printf("exit before ...");
    //_exit(0); //并没有打印
    exit(0);
    printf("exit after ...");
}

//孤儿进程与僵尸进程
//孤儿进程
//此时子进程由于执行 while 死循环并没有退出，此时处于运行态（R），唯一需要关注的是其父进程 ID 为1。ID 为1的进程是 init 进程，是 Linux 系统在应用空间上运行的第一个进程
//进程在退出时，通常由该进程的父进程对其资源进行回收及释放资源。如果该进程的父进程提前退出，那么此时该进程将失去了“父亲”，则成为了“孤儿” 。
//因此系统默认 init 进程成为孤儿进程的“ 父亲” ，以便于在以后对其资源进行回收。
void orphan_test() {
    pid_t pid;

    pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        //子进程死循环，没有退出
        cout << "child" << getpid() << " " << getppid() << endl;
        while (1) {
            sleep(1);
        }
    } else {
        //父进程打印输出后，就退出了
        cout << "parent" << getpid() << " " << getppid() << endl;
    }
}

//僵尸进程
//僵尸进程其实是进程的一种状态，即僵尸态。
//进程的僵尸态与死亡态很接近。 唯一不同的是，死亡进程，即进程退出，释放所有资源；而僵尸进程，即进程退出但没有释放资源。
void zombie_test() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
    }

    if (pid == 0) {
        //子进程退出
        //子进程的状态为 Z+, Z 表示僵尸态
        cout << "child" << getpid() << " " << getppid() << endl;
    } else {
        //父进程不退出
        //如果父进程不退出子进程退出，此时父进程不会主动回收子进程的资源，子进程成为僵尸进程。
        cout << "parent" << getpid() << " " << getppid() << endl;

        while (1) {
            sleep(1);
        }
    }
}

//wait()函数和 waitpid()函数
//进程的僵尸态与死亡态区别在于是否回收资源
//不会产生僵尸进程。这说明了两种可能性：
//一种是如果子进程先退出，父进程后退出，那么退出的父进程会将子进程的资源回收，那么不会产生僵尸进程；
//另一种是父进程先退出，子进程成为孤儿进程，孤儿进程退出，资源将会被 init 进程回收。

//在 Linux 中，通常可以选择 wait()函数及 waitpid()函数用来完成对僵尸进程的资源的回收。
//pid_t wait(int *status);
void wait_test() {
    pid_t pid;
    int i = 3;

    pid = fork();

    if (pid < 0) {
        perror("fork");
    }

    if (pid == 0) {
        cout << "child" << getpid() << " " << getppid() << endl;
        while (i > 0) {
            sleep(1);
            cout << i << endl;
            i--;
        }
    } else {
        int status;
        wait(&status); //使父进程阻塞,回收子进程
        cout << "parent" << getpid() << " " << getppid() << endl;
        while (1) {
            sleep(1);
        }
    }
}

//waitpid()函数
//如果父进程已经创建了多个子进程，使用 wait()函数将无法等到某个特定的子进程的完成，只能按顺序等待下一个子进程的终止。
//waitpid()函数被用来关注子进程的状态是否发生变化。
void waitpid_test() {
    pid_t pid;
    int i = 5;
    pid = fork();
    if (pid < 0) {
        perror("fork");
    }
    if (pid == 0) {
        cout << "child" << getpid() << " " << getppid() << endl;

        while (i > 0) {
            sleep(1);
            cout << i << endl;
            i--;
        }
    } else {
        pid_t ret;
        int status;
        while ((ret = waitpid(pid, &status, WNOHANG)) == 0) {
            sleep(1);
            cout << "子进程还没退出" << endl;
        }

        if (ret == pid) {
            cout << "子进程退出" << endl;
        }

        cout << "parent" << getpid() << " " << getppid() << endl;

        while (1) {
            sleep(1);
        }
    }
}

//Linux 守护进程
//Linux 守护进程又被称为 Daemon 进程，为 Linux 的后台服务进程（独立于控制终端）。
//1．创建子进程（子进程不退出，父进程退出）
//由于父进程先于子进程退出，造成子进程成为孤儿进程。此时子进程的父进程变成 init进程。
//2．在子进程中创建新会话
//使用的函数是 setsid()。该函数将会创建一个新会话，并使进程担任该会话组的组长。 同时，在会话组中创建新的进程组，该进程依然也是进程组的组长。
//该进程成为新会话组和进程组中唯一的进程。 最后使该进程脱离终端的控制，运行在后台。
//3．改变当前的工作目录
//使用 fork()函数创建的子进程继承了父进程的当前工作目录。系统通常的做法是让根目录成为守护进程的当前工作目录。 改变工作目录的函数是 chdir()。
//4．重设文件权限掩码
//使用函数 umask()，改变文件权限掩码，参数即为要修改的掩码值。
//5．关闭文件描述符
//标准输入、输出、错误输出的文件描述符 0、1、2，已经没有了任何价值，应当关闭。
void daemon_test() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid > 0) {
        exit(0); //退出父进程
    }

    setsid(); // 子进程成为新的会话组长和进程组长

    chdir("/"); // 修改当前工作路径

    umask(0); // 重设文件权限掩码

    // 关闭所有文件描述符
    for (int i = 0; i < getdtablesize(); i++) {
        close(i);
    }

    int fd;
    if ((fd = open("/tmp/daemon.log", O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
        perror("open");
        return;
    }
    while (1) {
        sleep(1);
        write(fd, "daemon", strlen("daemon"));
    }
    close(fd);
}

//系统日志
//void openlog(const char *ident, int option, int facility);
//void syslog(int priority, const char *format, ...);
//void closelog(void);
void syslog_test() {
    pid_t pid;

    pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid > 0) {
        exit(0); //退出父进程
    }

    openlog("daemon_syslog", LOG_PID, LOG_DAEMON);

    setsid(); // 子进程成为新的会话组长和进程组长

    chdir("/"); // 修改当前工作路径

    umask(0); // 重设文件权限掩码

    // 关闭所有文件描述符
    for (int i = 0; i < getdtablesize(); i++) {
        close(i);
    }

    int fd;
    if ((fd = open("/tmp/daemon.log", O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0) {
        perror("open");
        return;
    }

    while (1) {
        sleep(1);

        //日志会写到 /var/log/messages
        syslog(LOG_INFO, "daemon\n");
    }

    closelog();
    close(fd);
}

int main() {
    //fork_test();
    //exec_test();
    //exit_test();
    //orphan_test();
    //zombie_test();
    //wait_test();
    //waitpid_test();
    //daemon_test();
    syslog_test();
    return 0;
}
