#include <csignal>
#include <iostream>
#include <event.h>
using namespace std;

//高性能IO框架库 libevent
//linux服务器必须处理的三类事件：IO事件，信号，定时事件
//1、统一事件源
//2、可移植性
//3、对并发编程的支持

//基于 Reactor 模式
//组件：句柄(handle)，事件多路分发器(eventDemultiplexer)，事件处理器(eventHandler)，具体事件处理器(concreteEventHandler)

//安装
//git clone https://github.com/libevent/libevent.git
//cd libevent
//mkdir build && cd build
//cmake ..
//make
//sudo make install

void signal_cb(int fd, short event, void *argc) {
    event_base *base = (event_base *) argc;
    timeval delay = {2, 0};
    printf("an interrupt signal\n");
    event_base_loopexit(base, &delay); //延迟退出
}

void timeout_cb(int fd, short event, void *argc) {
    printf("timeout\n");
}

void event_test() {
    event_base *base = event_init(); //创建event_base对象，相当于一个Reactor实例

    //创建信号事件处理器
    event *signal_event = evsignal_new(base, SIGINT, signal_cb, base);
    event_add(signal_event, NULL);

    //创建定时事件处理器
    timeval tv = {1, 0};
    event *timeout_event = evtimer_new(base, timeout_cb, NULL);
    event_add(timeout_event, &tv);

    event_base_dispatch(base);

    event_free(timeout_event);
    event_free(signal_event);

    event_base_free(base);
}

int main() {
    event_test();
    return 0;
}
