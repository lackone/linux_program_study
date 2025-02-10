#include <iostream>
#include <unistd.h>
#include <sys/wait.h>

#include "sem_lock.h"
#include "mutex_lock.h"
#include "cond_lock.h"
using namespace std;

//多线程编程

//创建线程和结束线程
//int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void*(*start_routine)(void*), void* arg);
//void pthread_exit(void* retval);

//pthread_join
//int pthread_join(pthread_t thread, void ** retval);
//一个进程中的所有线程都可以调用pthread_join函数来回收其他线程，即等待其他线程结束。

//pthread_cancel
//异常终止一个线程，取消线程
//int pthread_cancel(pthread_t thread);

//收到取消请求的线程可以决定是否允许被取消，以及如何取消
//int pthread_setcancelstate(int state, int *oldstate);  //state取消状态，是否允许取消
//PTHREAD_CANCEL_ENABLE  允许取消
//PTHREAD_CANCEL_DISABLE  禁止取消

//int pthread_setcanceltype(int type, int *oldtype); //type取消类型，如何取消
//PTHREAD_CANCEL_ASYNCHRONOUS  线程随时都可以被取消
//PTHREAD_CANCEL_DEFERRED   允许线程推迟行动，直到取消点函数

//线程属性
//int pthread_attr_init(pthread_attr_t* attr);   //初始化属性
//int pthread_attr_destroy(pthread_attr_t *attr);  //销毁属性

//detachstate，线程脱离状态
//stackaddr 和 stacksize  线程堆栈的起始地址和大小
//guardsize   保护区域大小 创建线程时会在其堆栈尾部额外分配guardsize字节的空间
//schedparam  线程调度参数，类型sched_param结构体，只有一个成员sched_priority，表示运行优先级
//schedpolicy  线程调度策略，SCHED_OTHER默认值，SCHED_RR轮转算法，SCHED_FIFO先进先出
//inheritsched 是否继承调用线程的调度属性
//scope   线程间竞争CPU的范围，即线程优先级的有效范围

//POSIX信号量
//int sem_init(sem_t *sem, int pshared, unsigned int value);
//int sem_destroy(sem_t *sem);
//int sem_wait(sem_t *sem);
//int sem_trywait(sem_t *sem);
//int sem_post(sem_t *sem);

//互斥锁
//int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);
//int pthread_mutex_destroy(pthread_mutex_t *mutex);
//int pthread_mutex_lock(pthread_mutex_t *mutex);
//int pthread_mutex_trylock(pthread_mutex_t *mutex);
//int pthread_mutex_unlock(pthread_mutex_t *mutex);

//互斥锁的属性
//int pthread_mutexattr_init(pthread_mutexattr_t *attr);
//int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
//int pthread_mutexattr_getpshared(const pthread_mutexattr_t *attr, int *pshared);  //获取pshared属性
//int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr, int pshared);
//int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type); //获取type属性
//int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
//pshared 是否允许跨进程共享互斥锁  PTHREAD_PROCESS_SHARED  PTHREAD_PROCESS_PRIVATE
//type  指定互斥锁的类型
//PTHREAD_MUTEX_NORMAL      普通锁
//PTHREAD_MUTEX_ERRORCHECK  检错锁
//PTHREAD_MUTEX_RECURSIVE   嵌套锁
//PTHREAD_MUTEX_DEFAULT     默认锁

//死锁例子
//死锁产生的原因是，双方占着资源，不释放，又去向对方拿别人已经锁的资源
int a = 0;
int b = 0;
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;

void *routine(void *arg) {
    pthread_mutex_lock(&mutex_b); //子线程占有锁b
    printf("get mutex b\n");
    sleep(5);
    ++b;
    pthread_mutex_lock(&mutex_a); //去拿锁a
    b += a++;
    pthread_mutex_unlock(&mutex_a);
    pthread_mutex_unlock(&mutex_b);
    pthread_exit(NULL);
}

void deadlock_test() {
    pthread_t t;
    pthread_mutex_init(&mutex_a, NULL);
    pthread_mutex_init(&mutex_b, NULL);
    pthread_create(&t, NULL, routine, NULL);

    pthread_mutex_lock(&mutex_a); //主线程占有锁a
    printf("get mutex a\n");
    sleep(5);
    ++a;
    pthread_mutex_lock(&mutex_b); //去拿锁b
    a += b++;
    pthread_mutex_unlock(&mutex_b);
    pthread_mutex_unlock(&mutex_a);

    pthread_join(t, NULL);
    pthread_mutex_destroy(&mutex_a);
    pthread_mutex_destroy(&mutex_b);
}

//条件变量
//互斥锁是用于同步线程对共享数据的访问
//条件变量，是用于在线程之间同步共享数据的值。
//int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *cond_attr);
//int pthread_cond_destroy(pthread_cond_t *cond);
//int pthread_cond_broadcast(pthread_cond_t *cond);
//int pthread_cond_signal(pthread_cond_t *cond);
//int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

//线程同步机制包装类

//可重入函数
//如果一个函数能被多个线程同时调用而不发生竞态条件，则我们称它是线程安全的。或都说是 可重入函数。
// 可重入版本的函数名 在原函数名尾部加上 _r

//线程和进程
//某个线程调用了fork函数，新创建的子进程只会拥有一个执行线程，是调用fork那个线程的完整复制。子进程将自动继承父进程中互斥锁的状态。
//父进程中被加锁的互斥锁，在子进程中也是锁住的。如果子进程再次对该锁加锁，就会导致死锁。

pthread_mutex_t mux;

void *routine2(void *arg) {
    printf("child thread lock mux\n");
    pthread_mutex_lock(&mux); //这里上锁了
    sleep(5);
    pthread_mutex_unlock(&mux);
    return NULL;
}

void prepare() {
    printf("Prepare: Acquiring lock\n");
    pthread_mutex_lock(&mux);
}

void parent() {
    printf("Parent: Releasing lock\n");
    pthread_mutex_unlock(&mux);
}

void child() {
    printf("Child: Releasing lock\n");
    pthread_mutex_unlock(&mux);
}

void parent_child_lock() {
    pthread_mutex_init(&mux, NULL);
    pthread_t t;
    pthread_create(&t, NULL, routine2, NULL);

    //确保在fork之前，子线程已经获得了互斥锁
    sleep(1);

    pthread_atfork(prepare, parent, child);

    int pid = fork();
    if (pid < 0) {
        pthread_join(t, NULL);
        pthread_mutex_destroy(&mux);
        return;
    } else if (pid == 0) {
        //子进程继承了互斥锁的状态，处于锁定状态，父进程中子线程pthread_mutex_lock引起的
        //因此，下面会一直阻塞
        pthread_mutex_lock(&mux);
        printf("i can not run to here\n");
        pthread_mutex_unlock(&mux);
        exit(0);
    } else {
        wait(NULL);
    }
    pthread_join(t, NULL);
    pthread_mutex_destroy(&mux);
}

//pthread提供了一个专门的函数pthread_atfork，以确保fork调用后父进程和子进程都有一个清楚的锁状态
//int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));

//prepare: 在 fork() 创建子进程之前调用的函数。通常用于获取锁或其他资源。
//parent: 在 fork() 成功后，在父进程中调用的函数。通常用于释放或恢复在 prepare 中获取的资源。
//child: 在 fork() 成功后，在子进程中调用的函数。通常用于重新初始化或设置子进程中需要的状态。

//线程和信号
//每个线程可以独立地设置信号掩码
//int pthread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask);
//在一个线程中设置了某个信号的信号处理函数后，将覆盖其他线程为同一信号设置的信号处理函数。
//1、在主线程创建出其他子线程之前调用 pthread_sigmask 来设置信号掩码，所有新建的子线程都将自动继承这个信号掩码。
//2、在某个线程中调用如下函数来等待信号并处理
//int sigwait(const sigset_t *set, int *sig);

//可以明确地将一个信号发送给指定的线程
//int pthread_kill(pthread_t thread, int sig);

//用一个线程处理所有信号

#define handle_error_on(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void *sig_thread1(void *arg) {
    sigset_t *set = (sigset_t *) arg;
    int ret, sig;
    for (;;) {
        ret = sigwait(set, &sig);
        if (ret != 0) {
            handle_error_on(ret, "sigwait");
        }
        printf("thread %lu signal handling thread get signal %d\n", pthread_self(), sig);
    }
}

void *sig_thread2(void *arg) {
    for (;;) {
        sleep(1);
        printf("sig_thread2\n");
    }
}

void sig_thread_test() {
    pthread_t t1;
    pthread_t t2;
    sigset_t set;

    //信号掩码决定了哪些信号会被阻塞，并且不会传递给该线程。
    sigemptyset(&set); //置空
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGUSR1);
    int ret = pthread_sigmask(SIG_BLOCK, &set, NULL); //添加信号掩码
    if (ret != 0) {
        handle_error_on(ret, "pthread_sigmask");
    }
    pthread_create(&t1, NULL, sig_thread1, (void *) &set);
    pthread_create(&t2, NULL, sig_thread2, (void *) &set);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
}

int main() {
    //deadlock_test();
    //parent_child_lock();
    sig_thread_test();
    return 0;
}
