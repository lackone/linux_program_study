#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string>
using namespace std;

//I/O 的定义
//它所指的是 Input/Output，即输入/输出。I/O 操作的基本对象为文件，文件既可以是设备文件，也可以是普通文件。
//I/O 的类型
//标准 I/O 与文件 I/O
//I/O 的操作模式
//阻塞 I/O、非阻塞 I/O、多路复用 I/O 以及异步 I/O

//系统调用
//系统调用接口层（System Call Interface，SCI）介于应用层与内核层之间（系统调用接口层不属于内核层，但它是由内核函数实现的）。
//系统调用执行的流程
//（1）应用程序代码调用封装的 func()函数，该函数是一个包装的系统调用的函数。
//（2）func()函数负责准备向内核传递参数，并触发软中断 int 0x80 切换到内核。
//（3）CPU 被软中断打断后，执行中断处理函数，即系统调用处理函数（system_call）。
//（4）system_call 调用系统调用服务例程（sys_func()），真正开始处理该系统调用。

//文件 I/O
//就是操作系统封装了一系列函数接口供应用程序使用，通过这些接口可以实现对文件的读写操作。
//标准 I/O
//函数接口在对文件进行操作时，首先操作缓存区，等到缓存区满足一定的条件时，然后再去执行系统调用，真正实现对文件的操作。

//标准 I/O 的操作核心
//标准 I/O 的操作都是围绕流（stream）来进行的。在标准 I/O 中，流用 FILE*来描述。
//FILE *fopen(const char *path, const char *mode);
//FILE *fdopen(int fd, const char *mode);
//FILE *freopen(const char *path, const char *mode, FILE *stream);
//int fclose(FILE *fp);

//当用户程序运行时，系统会自动打开 3 个流指针，它们分别是：
//标准输入流指针 stdin     0
//标准输出流指针 stdout    1
//标准错误输出流指针 stderr  2

//错误处理
//perror(const char *s);
//系统会将错误码保存在全局变量 errno中
//char *strerror(int errnum);

//流的读写
//int fputc(int c, FILE *stream);  向指定的流中写入一个字符
//int fgetc(FILE *stream);  从指定的流中读取一个字符
//int fputs(const char *s, FILE *stream); 向指定的流中写入字符串
//char *fgets(char *s, int size, FILE *stream);  从指定的流中读取字符串

//按数据大小的形式输入/输出
//size_t fwrite(const void *ptr, size_t size, size_t nmemb,FILE *stream);
//size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

//系统预定义流指针
//stdin 作为标准输入流指针，默认是终端输入，而 stdout、stderr 同属于标准输出，默认是终端输出

//getchar()、putchar()、gets()、puts()等。 函数默认使用流指针
//getchar()函数用于读取标准输入流指针，每次操作一个字符，效果相当于fgetc (stdin)。
//putchar()函数用于向标准输出流指针写入（向终端输出），每次操作一个字符，效果相当于fputc (stdout)。
//gets()函数是一个不推荐使用的函数。
//puts()函数用于向标准输出流指针写入字符串

//标准 I/O 提供了三种类型的缓存，即全缓存、行缓存和不缓存。
//（1）全缓存。当流与文件相关联时，所操作的缓存区为全缓存。通俗地讲，即当使用标准 I/O函数操作流指针，而该流指针是与文件有关联的流指针时，访问的缓存区为全缓存区。
//（2）行缓存。当流与终端相关联时，所操作的缓存区为行缓存。
//（3）不缓存（没有缓存区）。当使用标准 I/O 函数操作的流指针是不带缓存区的流指针时，没有缓存区。

//缓存区的刷新及配置
//刷新全缓存的条件是缓存区写满、强制刷新（fflush()）、程序正常退出。
//而行缓存的刷新条件是缓存区写满、 强制刷新（fflush()）、 程序正常退出、 换行符‘ \n’ 。

//void setbuf(FILE *stream, char *buf);
//int setvbuf(FILE *stream, char *buf, int mode, size_t size);
//通过 mode 参数精确设置所需的缓存类型。_IOFBF 指定为全缓存，_IOLBF指定为行缓存，_IONBF 指定为不缓存。

//流的定位
//fseek()函数和 ftell()函数被用来实现读写位置的定位及位置的查询
//int fseek(FILE *stream, long offset, int whence);
//long ftell(FILE *stream);

//格式化输入/输出
//int scanf(const char *format, ...);
//int fscanf(FILE *stream, const char *format, ...);
//int sscanf(const char *str, const char *format, ...);
//int printf(const char *format, ...);
//int fprintf(FILE *stream, const char *format, ...);
//int sprintf(char *str, const char *format, ...);

int get_line(const char *file_name) {
    FILE *fp = fopen(file_name, "r");
    char buf[32] = {0};
    int cnt = 0;
    while (fgets(buf, 32, fp) != nullptr) {
        if (buf[strlen(buf) - 1] == '\n') {
            cnt++;
        }
    }
    fclose(fp);
    return cnt;
}

int get_line2(const char *file_name) {
    FILE *fp = fopen(file_name, "r");
    char c = 0;
    int cnt = 0;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '\n') {
            cnt++;
        }
    }
    fclose(fp);
    return cnt;
}

void file_test() {
    FILE *fp;
    if ((fp = fopen("test.txt", "rb")) == nullptr) {
        cout << "fopen error: " << strerror(errno) << endl;
        perror("fopen error");
        cout << "打开文件失败" << endl;
    } else {
        fclose(fp);
    }

    if ((fp = fopen("test.txt", "wb")) == nullptr) {
        perror("fopen error");
    }
    if (fputc('a', fp) == EOF) {
        perror("fputc error");
    }
    fclose(fp);


    fp = fopen("test.txt", "rb");
    fseek(fp, 0, SEEK_SET);
    char c = fgetc(fp);
    cout << c << endl;

    fclose(fp);
}

struct data {
    int a;
    char b;
    char buf[32];
};

void write_test() {
    struct data d = {
        .a = 1,
        .b = 'a',
        .buf = "test",
    };
    FILE *fp = fopen("test.txt", "wb");
    fwrite(&d, sizeof(d), 1, fp);
    fclose(fp);
}

void read_test() {
    FILE *fp = fopen("test.txt", "rb");
    struct data d;
    fread(&d, sizeof(d), 1, fp);
    cout << d.a << endl;
    cout << d.b << endl;
    cout << d.buf << endl;
    fclose(fp);
}

void std_test() {
    int ch;

    while ((ch = fgetc(stdin)) != EOF) {
        fputc(ch, stdout);
        if (ch == 'c') {
            break;
        }
    }
}

void buffer_size_test() {
    FILE *fp = fopen("test.txt", "rb");

    //fp 是与文件关联的流指针，stdin、stdout、stderr 都是与终端关联的流指针
    //全缓存的大小为 4KB，行缓存为 1KB。
    cout << "fp :" << (fp->_IO_buf_end - fp->_IO_buf_base) << endl; //4KB
    cout << "stdin :" << (stdin->_IO_buf_end - stdin->_IO_buf_base) << endl; //1KB
    cout << "stdout :" << (stdout->_IO_buf_end - stdout->_IO_buf_base) << endl; //1KB
    cout << "stderr :" << (stderr->_IO_buf_end - stderr->_IO_buf_base) << endl; //0

    fclose(fp);
}

void seek_test() {
    FILE *fp = fopen("test.txt", "a+");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    cout << "size : " << size << endl;
    fputs("aabbcc", fp);
    fclose(fp);
}

//linux文件I/O
//文件描述符
//对于 Linux 而言，所有对设备和文件的操作都是通过文件描述符来进行的。

//文件的打开和关闭
//int open(const char *pathname, int flags);
//int open(const char *pathname, int flags, mode_t mode);
//int close(int fd);

//一个进程最多可以使用 1024 个文件描述符。
//文件读写
//ssize_t write(int fd, const void *buf, size_t count);
//ssize_t read(int fd, void *buf, size_t count);

//文件定位
//off_t lseek(int fd, off_t offset, int whence);

//文件控制操作
//文件上锁的函数有 lockf()和 fcntl()
//int fcntl(int fd, int cmd, ... /* arg */ );

void open_test() {
    int fd;

    fd = open("aaa.txt", O_RDONLY | O_WRONLY | O_CREAT | O_TRUNC);
    cout << "fd :" << fd << endl;

    struct data d = {
        .a = 2,
        .b = 'c',
        .buf = "test",
    };
    write(fd, &d, sizeof(d));

    close(fd);
}

int lock_fd(int fd, int type) {
    struct flock old_lock, lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = type;
    lock.l_pid = -1;

    //第一次使用 F_GETLK 命令判断是否可以执行 flock 结构体所描述的锁操作
    fcntl(fd, F_GETLK, &lock);

    if (lock.l_type != F_UNLCK) {
        if (lock.l_type == F_RDLCK) {
            cout << "已存在读锁" << lock.l_pid << endl;
        } else if (lock.l_type == F_WRLCK) {
            cout << "已存在写锁" << lock.l_pid << endl;
        }
    }

    lock.l_type = type;

    //第二次用F_SETLK或F_SETLKW命令设置 flock 结构体所描述的锁操作
    //阻塞
    if (fcntl(fd, F_SETLKW, &lock) < 0) {
        return -1;
    }

    switch (lock.l_type) {
        case F_RDLCK:
            cout << "读锁" << getpid() << endl;
            break;
        case F_WRLCK:
            cout << "写锁" << getpid() << endl;
            break;
        case F_UNLCK:
            cout << "解锁" << getpid() << endl;
            return 1;
            break;
    }
    return 0;
}

void lock_test() {
    int fd = open("test.txt", O_RDWR);

    lock_fd(fd, F_WRLCK);
    getchar();
    lock_fd(fd, F_UNLCK);
    getchar();
}

//生产者
int producer() {
    int fd;
    char buf[32] = {0};
    static int cout = 0;

    if ((fd = open("fifo", O_RDWR | O_CREAT|O_APPEND, 0664)) < 0) {
        perror("open fifo");
        return -1;
    }

    sprintf(buf, "%c", 'a' + cout);
    cout = (cout + 1) % 26;

    lock_fd(fd, F_WRLCK);

    printf("写入 %s\n", buf);

    if (write(fd, buf, strlen(buf)) < 0) {
        perror("write");
        return -1;
    }

    lock_fd(fd, F_UNLCK);

    close(fd);
    return 0;
}

int consumer() {
    int fd;
    char buf;
    if ((fd = open("fifo", O_RDONLY)) < 0) {
        perror("open fifo");
        return -1;
    }

    lseek(fd, 0, SEEK_SET);

    int i = 0;
    while (i < 10) {
        while ((read(fd, &buf, 1) == 1) && i < 10) {
            fputc(buf, stdout);
            i++;
        }
    }

    close(fd);
    return 0;
}

int main(int argc, char *argv[]) {
    //file_test();

    cout << get_line("test.txt") << endl;
    cout << get_line2("test.txt") << endl;

    write_test();
    read_test();
    //std_test();
    buffer_size_test();
    seek_test();

    open_test();
    lock_test();

    if (strcmp(argv[1], "prd") == 0) {
        cout << "producer" << endl;
        int i = 0;
        while (i < 10) {
            producer();
            sleep(1);
            i++;
        }
    } else if (strcmp(argv[1], "con") == 0) {
        cout << "consumer" << endl;
        consumer();
    }

    return 0;
}
