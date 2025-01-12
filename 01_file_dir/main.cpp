#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>
#include <libgen.h>
using namespace std;

//对于 Linux 操作系统而言，一切皆文件
//文件的类型
//（1）普通文件（regular file）。这种文件是最常见的文件类型，其数据形式可以是文本或二进制数据。
//（2）目录文件（directory file）。这种文件包含其他类型文件的名字以及指向与这些文件有关的信息的指针。 对一个目录文件有读许可权的任一进程都可以读该目录文件的内容，但只有内核才有写目录文件的权限。
//（3）字符设备文件（character special file）。这种文件被视为对字符设备的一种抽象，它代表的是应用程序对硬件设备的访问接口。Linux应用程序通过对该文件进行操作来实现对设备的访问。
//（4）块设备文件（block special file）。这种文件类似于字符设备文件，只是它用于磁盘设备。Linux系统中的所有设备或者抽象为字符设备文件，或者为块设备文件。
//（5）管道文件（pipe file）。这种文件用于进程间的通信，有时也将其称为命名管道。
//（6）套接字文件（socket file）。这种文件用于进程间的网络通信，也可用于在一台宿主机上的 进程之间的本地通信。
//（7）符号链接文件（symbolic link file）。这种文件指向另一个文件。

//命令 ls -l 查看目录下所有文件及类型
//b     块设备文件
//c     字符设备文件
//d     目录文件
//-     普通文件
//l     符号链接文件
//s     套接字文件
//p     管道文件

//符号链接文件
//符号链接文件类似于 Windows 系统的快捷方式，只保留目标文件的地址，而不用占用存储空间。
//Linux 中有两种类型的链接：硬链接和软链接。
//硬链接是利用 Linux 系统为每个文件分配的物理编号 i 节点建立链接。这种方式类似于 Windows 系统中将文件复制一份。
//软链接是利用文件的路径名建立链接。
//不同之处：
//源文件删除后，软链接无法定位到源文件，所以会显示没有文件；硬链接类似于复制，删除源文件，硬链接依然可以访问。

//1．link()函数和 unlink()函数分别用来创建和移除硬链接
//2．symlink()函数用来创建软链接
//3. readlink()函数则可以获取链接本身的内容，即其所指向的路径名。

//stat()函数、fstat()函数和 lstat()函数

//int stat(const char *path, struct stat *buf); 函数得到一个与 path 所指定的文件有关的信息结构
//int fstat(int fd, struct stat *buf);
//int lstat(const char *path, struct stat *buf);  函数返回符号链接的有关信息

void stat_test() {
    struct stat buf;

    //获取文件属性
    if (stat("test.txt", &buf) == 0) {
        //所属用户的 ID
        //其用户所在的组 ID
        //文件的大小
        cout << buf.st_uid << " " << buf.st_gid << " " << buf.st_size << endl;
    }

    if (chown("test.txt", 0, 0) == 0) {
        cout << "修改 uid gid 成功" << endl;
        stat("test.txt", &buf);
        cout << buf.st_uid << " " << buf.st_gid << " " << buf.st_size << endl;
    }
}

//文件属主
//每个文件都有一个与之关联的用户 ID（UID）和组 ID（GID），即文件的属主和属组。
//系统调用 chown()函数、 lchown()函数和 fchown()函数来改变用户 ID 和组 ID。
//chown()函数用于改变由 path 参数指定的文件的属主。
//lchown()函数用途与 chown()函数相似，不同之处在于如果参数 path 为一符号链接，则将会改变 符号链接文件本身的所有权，而与该链接所指向的文件无关。
//fchown()函数也会改变文件的所有权，只是文件由打开文件描述符 fd 表示

//文件的存取许可权
// stat.st_mode 值指定了属主对文件的存取许可权
//S_IRUSR 所属用户可读
//S_IWUSR 所属用户可写
//S_IXUSR 所属用户可执行
//S_IRGRP 与所属用户同组的其他用户可读
//S_IWGRP 与所属用户同组的其他用户可写
//S_IXGRP 与所属用户同组的其他用户可执行
//S_IROTH 其他用户可读
//S_IWOTH 其他用户可写
//S_IXOTH 其他用户可执行


//chmod()函数和 fchmod()函数
// test.txt 的存取许可权是 rw-rw-r--，每三个符号为一组
//      rw-                    rw-                 rw-
//文件所属用户对文件的权限  同组其他用户对文件的权限  其他用户对文件的权限
//我们用 r 表示可读权限，用 w 表示可写权限，用 x 表示可执行权限，用-表示不具备该权限。
void chmod_test() {
    if (chmod("test.txt", 0777) == 0) {
        cout << "文件修改权限成功" << endl;
    }
}

//文件的长度
// stat.st_size 指定了以字节为单位的文件的长度。
//文件空洞不占用任何磁盘空间，直到某个时间点，文件空洞中写入数据时，文件系统才会为之分配磁盘块。 空洞的存在意味着一个文件名义上的大小可能比其占用的磁盘存储空间要大

//文件的截取
//截短文件，可以调用truncate()函数和 ftruncate()函数
void truncate_test() {
    if (truncate("test.txt", 5) == 0) {
        cout << "截短成功" << endl;
    }
}

//更改文件名
//rename()函数，既可以重命名文件，又可以将文件移至同一文件系统中的另一目录。
void rename_test() {
    if (rename("test.txt", "test.txt") == 0) {
        cout << "改名成功" << endl;
    }
}

//文件的时间戳
//stat 结构体中的成员 st_atime、st_mtime、st_ctime 字段表示文件时间戳，分别记录对文件的上次访问时间、 上次修改时间，以及文件状态上次发生变更的时间。
//utime()函数或与之相关的函数接口，可改变存储于文件 i 节点中的文件上次访问时间戳和上次修改时间戳。
//utime()函数和 utimes()函数之间最显著的差别在于后者可以以微秒级精度来指定时间值
//futimes()库函数和 lutimes()库函数的功能与 utimes()函数大同小异。
void utime_test() {
    struct stat buf;
    stat("test.txt", &buf);

    struct utimbuf time;

    time.actime = buf.st_atime;
    time.modtime = buf.st_atime;

    if (utime("test.txt", &time) == 0) {
        cout << "修改时间成功" << endl;
    }
}

//目录操作
//mkdir()函数和 rmdir()函数
//opendir()函数和 readdir()函数可以完成对目录的操作，实现对目录的读取。
void dir_test() {
    if (mkdir("aaa", 0777) == 0) {
        cout << "创建目录成功" << endl;
    }
    if (mkdir("aaa/bbb", 0777) == 0) {
        cout << "创建目录成功" << endl;
    }
    if (mkdir("aaa/bbb/ccc", 0777) == 0) {
        cout << "创建目录成功" << endl;
    }

    auto dir = opendir("aaa");
    if (dir != nullptr) {
        dirent *read;
        while (read = readdir(dir)) {
            cout << read->d_name << endl;
        }
    }
}

//解析路径名字符串
//dirname()函数和 basename()函数将一个路径名字符串分解成目录和文件名两部分。
void dirname_test() {
    cout << dirname("test.txt") << endl;
    cout << basename("test.txt") << endl;
}

//文件系统的类型
//1．磁盘文件系统
//2．网络文件系统
//3．虚拟文件系统

int main() {
    cout << "Hello World!" << endl;
    stat_test();
    chmod_test();
    truncate_test();
    rename_test();
    utime_test();
    dir_test();
    dirname_test();
    return 0;
}
