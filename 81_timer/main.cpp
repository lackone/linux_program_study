#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <list>
#include <signal.h>
#include <sys/epoll.h>
#include <queue>
using namespace std;

//定时器
//定时是指在一段时间之后触发某段代码的机制
//linux提供了三种定时方法：
//1、socket选项 SO_RCVTIMEO 和 SO_SNDTIMEO
//2、SIGALRM 信号
//3、IO复用系统调用的超时参数

//socket选项 SO_RCVTIMEO 和 SO_SNDTIMEO
//分别用来设置socket接收数据超时时间，和发送数据超时时间，仅对数据接收和发关相关socket调用有效
//send      SO_SNDTIMEO     返回-1，errno为 EAGAIN 或 EWOULDBLOCK
//sendmsg   SO_SNDTIMEO     返回-1，errno为 EAGAIN 或 EWOULDBLOCK
//recv      SO_RCVTIMEO     返回-1，errno为 EAGAIN 或 EWOULDBLOCK
//recvmsg   SO_RCVTIMEO     返回-1，errno为 EAGAIN 或 EWOULDBLOCK
//accept    SO_RCVTIMEO     返回-1，errno为 EAGAIN 或 EWOULDBLOCK
//connect   SO_SNDTIMEO     返回-1，errno为 EINPROGRESS

//设置connect超时时间
//
int connect_timeout(const char *ip, int port, int time) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv;
    tv.tv_sec = time;
    tv.tv_usec = 0;
    socklen_t len = sizeof(tv);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, len);

    int ret = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
    if (ret == -1) {
        if (errno == EINPROGRESS) {
            printf("连接超时\n");
            return -1;
        }
        printf("连接错误\n");
        return -1;
    }

    return sock;
}

//SIGALRM 信号
//由 alarm 和 setitimer 函数设置的实时闹钟，一旦超时，将触发SIGALRM信号

//升序定时器链表

//定时器类
struct client_data;

class util_timer {
public:
    time_t expire; //任务超时时间
    void (*callback_func)(client_data *); //回调函数
    client_data *data; //用户数据
};

//用户数据
struct client_data {
    sockaddr_in addr;
    int sock;
    char buf[1024];
    util_timer *timer;
};

class sort_timer_list {
public:
    sort_timer_list() {
    }

    ~sort_timer_list() {
        for (auto *v: time_list) {
            delete v;
        }
    }

    void print() {
        for (auto *v: time_list) {
            printf("%d-%d\t", v->data->sock, v->expire);
        }
        printf("\n");
    }

    //添加定时器
    void add_timer(util_timer *timer) {
        time_list.push_back(timer);
        sort_flush();
    }

    //修改定时器
    void adjust_timer(util_timer *timer) {
        auto it = find(time_list.begin(), time_list.end(), timer);
        if (it != time_list.end()) {
            //如果已经添加
            (*it)->expire = timer->expire;
        } else {
            //没有添加，则添加
            time_list.push_back(timer);
        }

        //重新刷新链表
        sort_flush();
    }

    void del_timer(util_timer *timer) {
        time_list.remove(timer);
    }

    //SIGALRM信号每次被触发就在其信号处理函数中执行一次tick函数，处理链表上到期的任务
    void tick() {
        if (time_list.empty()) {
            return;
        }
        print();
        //获取当前时间秒数
        time_t now = time(NULL);

        while (!time_list.empty()) {
            auto item = time_list.front();

            //判断当前时间
            if (now < item->expire) {
                break;
            }
            //调用回调函数
            item->callback_func(item->data);

            //释放内存
            delete item;

            // 移除列表中的第一个元素
            time_list.pop_front();
        }
    }

private:
    void sort_flush() {
        time_list.sort([](const util_timer *a, const util_timer *b) {
            return a->expire < b->expire;
        });
    }

    list<util_timer *> time_list;
};

//处理非活动连接
//
#define TIMESLOT 5

int sig_pipe[2]; //管道传递信号
sort_timer_list s_time_list; //升序链表管理定时器
int epfd = 0; //保存epoll


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

void add_sig(int sig) {
    struct sigaction act;
    act.sa_handler = sig_handler;
    act.sa_flags |= SA_RESTART;
    sigfillset(&act.sa_mask);
    sigaction(sig, &act, NULL);
}

void timer_handler() {
    s_time_list.tick(); //定时触发调用tick()

    //一次alarm调用只会引起一次SIGALRM信号，需要重新定时
    alarm(TIMESLOT);
}

//定时器回调函数，删除非活动连接
void time_callback(client_data *data) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, data->sock, 0);

    close(data->sock);

    printf("close fd %d\n", data->sock);
}

void timeout_server_test(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));
    listen(sock, 5);

    epoll_event evs[1024];
    epfd = epoll_create(1024);
    addfd(epfd, sock);

    socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipe);
    setnonblocking(sig_pipe[1]);
    addfd(epfd, sig_pipe[0]);

    //添加信号
    add_sig(SIGALRM);
    add_sig(SIGTERM);
    bool stop = false; //服务器停止

    client_data *users = new client_data[65535]; //用户数据
    bool timeout = false; //

    alarm(TIMESLOT);

    while (!stop) {
        int ret = epoll_wait(epfd, evs, 1024, -1);
        if (ret < 0 && errno != EINTR) {
            printf("epoll_wait error\n");
            break;
        }
        for (int i = 0; i < ret; i++) {
            int fd = evs[i].data.fd;
            if (fd == sock) {
                sockaddr_in client;
                socklen_t client_len = sizeof(client);
                int cfd = accept(sock, (struct sockaddr *) &client, &client_len);
                addfd(epfd, cfd);
                printf("client %d\n", cfd);

                users[cfd].addr = client;
                users[cfd].sock = cfd;

                util_timer *timer = new util_timer();
                timer->data = &users[cfd];
                timer->callback_func = time_callback;
                time_t now = time(NULL);
                timer->expire = now + 3 * TIMESLOT;

                users[cfd].timer = timer;

                s_time_list.add_timer(timer);
            } else if (fd == sig_pipe[0] && (evs[i].events & EPOLLIN)) {
                //有信号来了
                int sig;
                char signals[1024] = {0};
                ret = recv(fd, signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; i++) {
                        switch (signals[i]) {
                            case SIGALRM:
                                //标记有定时任务需要处理，但不立即处理，定时任务的优先级不是很高
                                timeout = true;
                                break;
                            case SIGTERM:
                                stop = true;
                                break;
                        }
                    }
                }
            } else if (evs[i].events & EPOLLIN) {
                //处理客户端上接收到数据
                memset(users[fd].buf, 0, sizeof(users[fd].buf));
                ret = recv(fd, users[fd].buf, sizeof(users[fd].buf) - 1, 0);

                util_timer *timer = users[fd].timer;
                if (ret < 0) {
                    if (errno != EAGAIN) {
                        //发生错误，关闭连接，删除定时器
                        time_callback(&users[fd]);
                        if (timer) {
                            s_time_list.del_timer(timer);
                        }
                    }
                } else if (ret == 0) {
                    //客户端关闭连接
                    time_callback(&users[fd]);
                    if (timer) {
                        s_time_list.del_timer(timer);
                    }
                } else {
                    printf("get %d bytes data %s from %d\n", ret, users[fd].buf, fd);

                    //如果客户端上数据可读，则我们需要调整连接对应的定时器
                    time_t now = time(NULL);
                    timer->expire = now + 3 * TIMESLOT;
                    printf("adjust timer \n");
                    s_time_list.adjust_timer(timer);
                }
            }
        }

        if (timeout) {
            timer_handler();
            timeout = false;
        }
    }

    close(sock);
    close(sig_pipe[1]);
    close(sig_pipe[0]);
    delete[] users;
}

//IO复用系统调用的超时参数
//由于IO复用系统调用可能在超时时间到期之前就返回，所以如果我们要利用它们来定时，就需要不断更新定时参数以反映剩余的时间
#define TIMEOUT 5000

void io_timeout_handler() {
    int timeout = TIMEOUT;

    time_t start = time(NULL);
    time_t end = time(NULL);

    epoll_event evs[1024];
    int epfd = epoll_create(1024);

    while (1) {
        start = time(NULL);
        int ret = epoll_wait(epfd, evs, 1024, timeout);
        if (ret < 0 && errno != EINTR) {
            printf("epoll_wait error\n");
            break;
        }
        if (ret == 0) {
            //返回0时，说明超时了
            timeout = TIMEOUT;
            continue;
        }
        end = time(NULL);
        //返回值大于0时，epoll_wait用时是 (end - start)*1000 毫秒
        //需要将 timeout 减去这段时间，
        timeout -= (end - start) * 1000;
        if (timeout <= 0) {
            timeout = TIMEOUT;
        }

        //处理....
    }
}

//高性能定时器
//基于排序链表的定时器存在一个问题，添加定时器的效率偏低

//时间轮
//指针指向轮子上的一个槽，以恒定的速度顺时针转动，每转动一步就指向下一个槽，每次转动称为一个滴答（tick），
//一个滴答的时间称为时间轮的槽间隔si(slog interval)，它实际上就是心搏时间。该时间轮共有N个槽，它运转一周的时间是 N*si，
//每个槽指向一条定时器链表，每条链表上的定时器具有相同的特征：它们的定时时间相差 N*si的整数倍
//假如指针指向槽 cs，添加一个定时时间为 ti 的定时器，则定时器将插入 ts 对应的链表中:
// ts = (cs + (ti/si)) % N

struct tw_client_data;

class tw_timer {
public:
    tw_timer(int circle, int slot) : circle_num(circle), time_slot(slot) {
    }

    time_t expire; //任务超时时间
    void (*callback_func)(tw_client_data *); //回调函数
    tw_client_data *data; //用户数据
    int time_slot; //定时器在时间轮的哪个槽
    int circle_num; //记录定时器在时间轮转多少圈后生效
};

//用户数据
struct tw_client_data {
    sockaddr_in addr;
    int sock;
    char buf[1024];
    tw_timer *timer;
};

class time_wheel {
public:
    time_wheel() : cur_slot(0) {
    }

    ~time_wheel() {
        //遍历每个槽，释放内存
        for (int i = 0; i < N; i++) {
            for (auto *v: slots[i]) {
                delete v;
            }
        }
    }

    tw_timer *add_timer(int timeout) {
        if (timeout < 0) {
            return NULL;
        }
        int tick_num = 0;
        if (timeout < si) {
            //如果超时时间小于一个滴答数，
            tick_num = 1;
        } else {
            tick_num = timeout / si;
        }

        //计算插入的定时器需要转多少圈后触发
        int circle_num = tick_num / N;
        //计算插入的定时器被插入到哪个槽中
        int time_slot = (cur_slot + (tick_num % N)) % N;
        //创建定时器
        tw_timer *timer = new tw_timer(circle_num, time_slot);

        //添加定时器
        slots[time_slot].push_back(timer);

        return timer;
    }

    void del_timer(tw_timer *timer) {
        if (!timer) {
            return;
        }
        int time_slot = timer->time_slot;
        slots[time_slot].remove(timer);
    }

    //SI时间到后，调用函数，时间轮向前滚动一个槽的间隔
    void tick() {
        printf("cur_slot = %d\n", cur_slot);
        for (auto it = slots[cur_slot].begin(); it != slots[cur_slot].end();) {
            if ((*it)->circle_num > 0) {
                (*it)->circle_num--;
                it++;
            } else {
                (*it)->callback_func((*it)->data);
                delete (*it);
                it = slots[cur_slot].erase(it);
            }
        }
        cur_slot = ++cur_slot % N;
    }

private:
    static const int N = 60; //槽的数目
    static const int si = 1; //槽间隔为1秒
    list<tw_timer *> slots[N]; //每个槽，都指向一个定时器链表
    int cur_slot; //当前所在的槽
};

//时间堆
//前面讨论的定时方案都是以固定的频率调用心搏函数tick，并在其中检测到期的定时器，然后执行定时器上的回调函数
//另外一种思路：
//将所有定时器中的超时时间最小的一个定时器的超时值作为心搏间隔，一旦心搏函数tick调用，超时时间最小的定时器必然到期，我们就可以在tick函数中处理该定时器。
//然后，再次从剩余的定时器中找出超时时间最小的一个，并将这段最小时间设置为下一次心搏间隔，如此反复，实现较为精确的定时。

struct mh_client_data;

class mh_timer {
public:
    mh_timer(int delay) {
        expire = time(NULL) + delay;
    }

    time_t expire; //任务超时时间
    void (*callback_func)(mh_client_data *); //回调函数
    mh_client_data *data; //用户数据
    bool deleted = false; //是否删除
};

struct mh_timer_compare {
    bool operator()(const mh_timer *lhs, const mh_timer *rhs) const {
        return lhs->expire > rhs->expire;
    }
};

//用户数据
struct mh_client_data {
    sockaddr_in addr;
    int sock;
    char buf[1024];
    mh_timer *timer;
};

class min_heap_timer {
public:
    min_heap_timer() {
    }

    ~min_heap_timer() {
        while (!min_heap.empty()) {
            auto v = min_heap.top();
            delete v;
            min_heap.pop();
        }
    }

    void add_timer(mh_timer *timer) {
        if (!timer) {
            return;
        }
        min_heap.push(timer);
    }

    void del_timer(mh_timer *timer) {
        if (!timer) {
            return;
        }
        timer->deleted = true;
    }

    //获取顶部定时器
    mh_timer *top() {
        while (!min_heap.empty() && min_heap.top()->deleted) {
            auto v = min_heap.top();
            delete v;
            min_heap.pop();
        }
        return min_heap.empty() ? NULL : min_heap.top();
    }

    //删除顶部定时器
    void pop_timer() {
        while (!min_heap.empty() && min_heap.top()->deleted) {
            auto v = min_heap.top();
            delete v;
            min_heap.pop();
        }
        if (!min_heap.empty()) {
            auto v = min_heap.top();
            delete v;
            min_heap.pop();
        }
    }

    void tick() {
        time_t now = time(NULL);
        while (!min_heap.empty()) {
            auto v = min_heap.top();

            if (v->deleted) {
                delete v;
                min_heap.pop();
                continue;
            }

            if (v->expire > now) {
                break;
            }

            if (v->callback_func) {
                v->callback_func(v->data);
            }

            delete v;

            min_heap.pop();
        }
    }

private:
    priority_queue<mh_timer *, vector<mh_timer *>, mh_timer_compare> min_heap;
};

int main(int argc, char *argv[]) {
    //connect_timeout(argv[1], atoi(argv[2]), 10);
    timeout_server_test(atoi(argv[1]));
    return 0;
}
