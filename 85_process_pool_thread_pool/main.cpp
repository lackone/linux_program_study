#include <iostream>
#include "cgi_conn.h"
using namespace std;

//进程池 和 线程池

//进程池 是由服务器预先创建的一组子进程，子进程数目在 3-10个之间，线程池中的线程数量应该和CPU数量差不多。
//进程池中所有子进程都运行着相同的代码，具有相同的属性，服务器启动之初就创建好了，每个子进程相对干净，没有打开不必要的文件描述符
//当有新任务来时，主进程通过某种方式选择池中的一个子进程来服务。
//选择子进程来服务，2种方式：
//1、主进程使用某种算法主动选择子进程，比如 随机算法 和 Round Robin轮流算法
//2、主进程 和 所有子进程通过一个共享的工作队列来同步，子进程都睡眠在该工作队列上，当有新任务来时，主进程添加任务到队列中，唤醒等待任务的子进程，不过只有一个子进程获得任务。

//处理多客户

//半同步/半异步进程池实现
//将接受新连接的操作放到子进程中，一个客户连接上的所有任务始终由一个子进程来处理

void cgi_conn_test(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    process_pool<cgi_conn> *pool = process_pool<cgi_conn>::instance(sock);
    if (pool) {
        pool->run();
        delete pool;
    }

    close(sock); //main函数创建的listen_fd，就需要main来关闭
}

//半同步/半反应堆线程池实现
//

int main() {
    cgi_conn_test(8888);
    return 0;
}
