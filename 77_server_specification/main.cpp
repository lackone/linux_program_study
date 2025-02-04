#include <fcntl.h>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

using namespace std;

//linux系统日志
//void syslog(int priority, const char* message, ...);

//改变syslog默认输出方式
//void openlog(const char* ident, int logopt, int facility);

//设置日志掩码
//int setlogmask(int maskpri);

//void closelog();

//用户信息
//UID       EUID      GID      EGID
//真实用户ID 有效用户ID 真实组ID  有效组ID
//uid_t getuid();
//uid_t geteuid();
//gid_t getgid();
//gid_t getegid();
//int setuid(uid_t uid);
//int seteuid(uid_t uid);
//int setgid(gid_t gid);
//int setegid(gid_t gid);
void uid_euid_test() {
    uid_t uid = getuid();
    uid_t euid = geteuid();
    //chmod +s 程序名
    //设置文件的set-user-id标志
    //uid是启动程序的用户的ID，euid是文件所有者的ID
    printf("uid = %d euid = %d\n", uid, euid);
}

//进程组
//每个进程都隶属于一个进程组，除PID信息外，还有进程组ID（PGID）
//pid_t getpgid(pid_t pid);

//设置PGID
//int setpgid(pid_t pid, pid_t pgid);

//会话
//一些有关联的进程组将形成一个会话session
//pid_t setsid(void);
//该函数不能由进程组的首领进程调用。非首领进程调用，效果如下：
//1、调用进程成为会话的首领，此时该进程是新会话的唯一成员。
//2、新建一个进程组，其PGID就是调用进程的PID，调用进程成为该组的首领。
//3、调用进程将甩开终端

//pid_t getsid(pid_t pid);

//用PS命令查看进程关系
//ps -o pid, ppid, pgid, sid, comm | less


//系统资源限制
//int getrlimit(int resource, struct rlimit *rlim);
//int setrlimit(int resource, const struct rlimit *rlim);

//改变工作目录和根目录
//char* getcwd(char* buf, size_t size); //获取当前工作目录
//int chdir(const char* path); //改变工作目录

//改变根目录
//int chroot(const char* path);

//服务器程序后台化
//linux提供了完成同样功能的库函数
//int daemon(int nochdir, int noclose);

bool daemonize() {
    //创建子进程
    pid_t pid = fork();
    if (pid < 0) {
        return false;
    } else if (pid > 0) {
        exit(0); //父进程退出
    }

    //设置文件权限掩码
    umask(0);
    //创建新会话，成为进程组首领
    pid_t sid = setsid();
    if (sid < 0) {
        return false;
    }
    //切换工作目录
    chdir("/");

    //关闭输入，输出，错误
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //将输入，输出，错误，重定向到/dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

    return true;
}

int main() {
    uid_euid_test();
    return 0;
}
