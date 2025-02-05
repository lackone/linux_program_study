#include <cstring>
#include <iostream>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

//服务器模型
//C/S模型
//所有客户端都通过访问服务器来获取所需的资源

//P2P模型
//让每台机器在消耗服务的同时也给别人提供服务

//两种高效的事件处理模式
//Reactor模式
//要求主线程只负责监听文件描述符上是否有事件发生，有的话立即将该事件通知工作线程，除此之外，主线程不做任何其他实质性的工作。
//读写数据，接受新的连接，以及处理客户请求均在工作线程中完成。
//使用同步IO模型实现的Reactor模式的工作流程：
//1、主线程往epoll内核事件表中注册socket上的读就绪事件
//2、主线程调用epoll_wait等待socket上有数据可读
//3、当socket上有数据可读时，epoll_wait通知主线程，主线程则将socket可读事件放入请求队列。
//4、睡眠在请求队列上的某个工作线程被唤醒，它从socket读取数据，并处理客户请求，然后往epoll内核事件表中注册该socket上的写就绪事件。
//5、主线程调用epoll_wait等待socket可写
//6、当socket可写时，epoll_wait通知主线程，主线程将socket可写事件放入请求队列。
//7、睡眠在请求队列上的某个工作线程被唤醒，它往socket上写入服务器处理客户端请求的结果。

//Proactor模式
//Proactor模式将所有IO操作都交给主线程和内核来处理，工作线程仅仅负责业务逻辑。
//1、主线程调用aio_read函数向内核注册socket上的读完成事件，并告诉内核用户读缓冲区的位置，以及读操作完成时如何通知应用程序
//2、主线程继续处理其他逻辑
//3、当socket上的数据被读入用户缓冲区后，内核将向应用程序发送一个信号，以通知应用程序数据已经可用。
//4、应用程序预先定义好的信号处理函数选择一个工作线程来处理客户请求，工作线程处理完客户请求之后，调用aio_write函数向内核注册socket上的写完成事件，
//并告诉内核用户写缓冲区的位置，以及写操作完成时如何通知应用程序。
//5、主线程继续处理其他逻辑
//6、当用户缓冲区的数据被写入socket之后，内核将向应用程序发送一个信号，以通知应用程序数据已经发送完毕。
//7、应用程序预先定义好的信号处理函数选择一个工作线程来做善后处理

//模拟Proactor模式
//使用同步IO模型模拟出的Proactor模式的工作流程如下
//1、主线程往epoll内核事件表中注册socket上的读就绪事件
//2、主线程调用epoll_wait等待socket上有数据可读
//3、当socket上有数据可读，epoll_wait通知主线程，主线程从socket循环读取数据，直到没有更多数据可读，然后将读取到的数据封装成一个请求对象并插入请求队列。
//4、睡眠在请求队列上的某个工作线程被唤醒，它获得请求对象并处理客户请求，然后往epoll内核事件表中注册socket上的写就绪事件
//5、主线程调用epoll_wait等待socket可写
//6、当socket可写时，epoll_wait通知主线程，主线程往socket上写入服务器处理客户请求的结果

//并发模式是指IO处理单元和多个逻辑单元之间协调完成任务的方法，主要有2种并发编程模式：半同步/半异步模式 和 领导者/追随者模式

//半同步/半异步模式
//IO模型中，同步和异步区分的是内核向应用程序通知的是何种IO事件，以及该由谁来完成IO读写
//并发模式中，同步指的是程序完全按照代码序列的顺序执行，异步指的是程序的执行需要由系统事件来驱动。
//半同步/半异步模式中，同步线程用于处理客户逻辑，异步线程用于处理IO事件

//领导者/追随者模式
//领导者/追随者模式是多个工作线程轮流获得事件源集合，轮流监听，分发并处理事件的一种模式。
//在任意时间点，程序都仅有一个领导者线程，它负责监听IO事件，而其他线程则都是追随者，它们休眠在线程池中等待成为新的领导者。
//当前的领导者如果检测到IO事件，首先要从线程池中推选出新的领导者线程，然后处理IO事件，此时，新的领导者等待新的IO事件，原来的领导者则处理IO事件，二者实现了并发。

//有限状态机

//状态独立的有限状态机
//状态机的每个状态都是相互独立的，没有相互转移
//STATE_MACHINE(Package _pack) {
//    PackageType _type = _pack.GetType();
//    switch(_type) {
//        case type_a:
//            process_package_a(_pack);
//            break;
//        case type_b:
//            process_package_b(_pack);
//            break;
//    }
//}

//带状态转移的有限状态机
//
//STATE_MACHINE() {
//    State cur_state = type_a;
//    while(cur_state != type_c) {
//        Package _pack = getNewPackage();
//        switch(cur_state) {
//            case type_a:
//                process_package_a(_pack);
//                cur_state = type_b;
//                break;
//            case type_b:
//                process_package_b(_pack);
//                cur_state = type_c;
//                break;
//        }
//    }
//}

//判断http头部结束的依据是遇到一个空行，该空行仅包含一对回车换行符
//如果一次读操作没有读入http请求的整个头部，即没有遇到空行，那么必须等待客户继续写数据并再次读入。

enum CHECK_STATE {
    CHECK_STATE_REQUEST_LINE = 0, //分析请求行
    CHECK_STATE_HEADER //分析头部字段
};

enum LINE_STATUS {
    LINE_OK = 0, //读取到完整行
    LINE_BAD, //行出错
    LINE_OPEN //行数据不完整
};

enum HTTP_CODE {
    NO_REQUEST,
    GET_REQUEST,
    BAD_REQUEST,
    FORBIDDEN_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};

const char *str_ret[] = {"i get a correct result\n", "something wrong\n"};

//解析一行内容
LINE_STATUS line_parse(char *buf, int &chk_index, int &read_index) {
    char tmp;
    //chk_index指向buf中当前正在分析的字节
    //read_index指向buf中客户数据的尾部的下一字节
    //buf中从0到chk_index字节已经分析完毕，第chk_index到read_index-1字节由下面的循环挨个分析
    for (; chk_index < read_index; chk_index++) {
        tmp = buf[chk_index]; //获取分析的字节
        if (tmp == '\r') {
            //如果\r是buf中最后一个数据，那么这次的分析没有读取到一个完整的行
            //返回LINE_OPEN表示还要继续读取客户数据
            if ((chk_index + 1) == read_index) {
                return LINE_OPEN; //
            } else if (buf[chk_index + 1] == '\n') {
                //如果下一个字符是\n，说明我们成功读取到一个完整的行
                buf[chk_index++] = '\0'; //把\r置0
                buf[chk_index++] = '\0'; //把\n置0
                return LINE_OK;
            }

            //否则说明请求有问题
            return LINE_BAD;
        } else if (tmp == '\n') {
            if (chk_index > 1 && buf[chk_index - 1] == '\r') {
                //如果前一个是\r，说明读取到完整的行
                buf[chk_index - 1] = '\0';
                buf[chk_index++] = '\0';
                return LINE_OK;
            }

            return LINE_BAD;
        }
    }

    //如果所有内容分析完毕，没有遇到\r，说明还要继续读取客户端数据
    return LINE_OPEN;
}

//分析请求行
HTTP_CODE request_line_parse(char *tmp, CHECK_STATE &chk_state) {
    char *url = strpbrk(tmp, " \t");
    //如果请求行中没有空白字符或\t，说明http请求有问题
    if (!url) {
        return BAD_REQUEST;
    }
    *url++ = '\0';

    char *method = tmp;
    //只支持GET方法
    if (strcasecmp(method, "GET") == 0) {
        printf("the request method is get\n");
    } else {
        return BAD_REQUEST;
    }
    //查找url中不在" \t"中出现的字符下标，相当于清空url前面的空白字符
    url += strspn(url, " \t");
    char *version = strpbrk(url, " \t");
    if (!version) {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    //仅支持HTTP/1.1
    if (strcasecmp(version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    //检查URL是否合法
    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }
    if (!url || url[0] != '/') {
        return BAD_REQUEST;
    }
    printf("the request url is %s\n", url);

    //请求行处理完毕，转移到头部字段的析
    chk_state = CHECK_STATE_HEADER;

    return NO_REQUEST;
}

//分析头部字段
HTTP_CODE headers_parse(char *tmp) {
    //遇到一个空行，说明我们得到了一个正确的HTTP请求
    if (tmp[0] == '\0') {
        return GET_REQUEST;
    } else if (strncasecmp(tmp, "Host:", 5) == 0) {
        tmp += 5;
        tmp += strspn(tmp, " \t");
        printf("the request host is %s\n", tmp);
    } else {
        printf("i can not handle this header\n");
    }
    return NO_REQUEST;
}

//分析HTTP请求的入口函数
HTTP_CODE content_parse(char *buf, int &chk_index, CHECK_STATE &chk_state, int &read_index, int &start_line) {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret_code = NO_REQUEST;

    //从buf中取出所有完整的行
    while ((line_status = line_parse(buf, chk_index, read_index)) == LINE_OK) {
        char *tmp = buf + start_line;
        start_line = chk_index; //记录下一行起始位置

        switch (chk_state) {
            case CHECK_STATE_REQUEST_LINE: {
                printf("line %s\n", tmp);
                ret_code = request_line_parse(tmp, chk_state);
                if (ret_code == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER: {
                printf("header %s\n", tmp);
                ret_code = headers_parse(tmp);
                if (ret_code == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (ret_code == GET_REQUEST) {
                    return GET_REQUEST;
                }
                break;
            }
            default: {
                return INTERNAL_ERROR;
            }
        }
    }

    //如果没有读取到一个完整的行，表示还需要继续读取客户端数据
    if (line_status == LINE_OPEN) {
        return NO_REQUEST;
    } else {
        return BAD_REQUEST;
    }
}

void server_test(int port) {
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

    char buf[4096] = {0};

    int len = 0;
    int read_index = 0; //已经读取的字节数
    int chk_index = 0; //已经分析的字节数
    int start_line = 0; //行在buf中的起始位置

    CHECK_STATE chk_state = CHECK_STATE_REQUEST_LINE;
    while (1) {
        len = recv(cfd, buf + read_index, sizeof(buf) - read_index, 0);
        if (len == -1) {
            printf("read error\n");
            break;
        } else if (len == 0) {
            printf("remote client has closed the connection\n");
            break;
        }
        read_index += len;
        //分析已经获得的客户端数据
        HTTP_CODE ret = content_parse(buf, chk_index, chk_state, read_index, start_line);
        if (ret == NO_REQUEST) {
            continue;
        } else if (ret == GET_REQUEST) {
            //得到一个完整的请求
            send(cfd, str_ret[0], strlen(str_ret[0]), 0);
            break;
        } else {
            //发生错误
            send(cfd, str_ret[1], strlen(str_ret[1]), 0);
            break;
        }
    }

    close(cfd);

    close(sock);
}

//提高服务器性能的其他建议
//池
//池是一组资源的集合，在服务器启动之初就被完全创建好并初始化，需要时直接从池中获取，处理完后，把资源放回池中。
//常见的有，内存池，进程池，线程池，连接池

//数据复制
//避免不必要的数据复制，2个工作进程之间要传递大量的数据时，考虑使用共享内存来在它们之间直接共享这些数据

//上下文切换
//进程切换和线程切换导致的系统开销，使用过多的工作线程，线程间的切换将占用大量的CPU时间

//锁
//共享资源的加锁保护，如果必须使用锁，则可以考虑减小锁的粒度，比如使用读写锁。

int main(int argc, char *argv[]) {
    server_test(atoi(argv[1]));
    return 0;
}
