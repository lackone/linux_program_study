#include <functional>
#include <iostream>
#include <list>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>

using namespace std;

//线程的基本概念
//线程是应用程序并发执行多种任务的一种机制。在一个进程中可以创建多个线程执行多个任务。
//每个进程都可以创建多个线程，创建的多个线程共享进程的地址空间，即线程被创建在进程所使用的地址空间上。
//同一个进程创建的线程共享进程的地址空间，因此创建线程比创建子进程要快得多。

//每个线程都拥有属于自己的栈区，用于存放函数的参数值、 局部变量的值、 返回地址等

//线程的创建
//int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

//一个线程可以通过 pthread_self()来获取自己的 ID
//pthread_t pthread_self(void);

//线程终止与回收
//（1）线程的执行函数执行 return 语句并返回指定值。
//（2）线程调用 pthread_exit()函数。
//（3）调用 pthread_cancel()函数取消线程。
//（4）任意线程调用 exit()函数，或者 main()函数中执行了 return 语句，都会造成进程中的所有线 程立即终止。

//pthread_exit()函数将终止调用线程，且参数可被其他线程调用 pthread_join()函数来获取。
//void pthread_exit(void *retval);

//pthread_join()函数用于等待指定 thread 标识的线程终止。
//int pthread_join(pthread_t thread, void **retval);

void *thread1(void *arg) {
    int cnt = *(int *) arg;
    while (cnt > 0) {
        cout << "thread1" << endl;
        sleep(1);
        cnt--;
    }
    pthread_exit((void *) "thread1 exit");
    return nullptr;
}

void *thread2(void *arg) {
    int cnt = *(int *) arg;
    while (cnt > 0) {
        cout << "thread2" << endl;
        sleep(1);
        cnt--;
    }
    pthread_exit((void *) "thread2 exit");
    return nullptr;
}

void thread_test() {
    pthread_t t1, t2;
    int arg1 = 5;
    pthread_create(&t1, NULL, thread1, &arg1);
    int arg2 = 5;
    pthread_create(&t2, NULL, thread2, &arg2);

    void *ret1;
    pthread_join(t1, &ret1);
    void *ret2;
    pthread_join(t2, &ret2);

    cout << (const char *) ret1 << endl;
    cout << (const char *) ret2 << endl;
}

//线程的分离
//pthread_detach 设置线程分离
//默认情况下，线程是可连接的（也可称为结合态）。通俗地说，就是当线程退出时，其他线程可以通过调用 pthread_join()函数获取其返回状态。
//int pthread_detach(pthread_t thread);
void *thread3(void *arg) {
    //一旦线程处于分离状态，就不能再使用 pthread_join()函数来获取其状态
    pthread_detach(pthread_self());

    int cnt = *(int *) arg;
    while (cnt > 0) {
        cout << "thread3" << endl;
        sleep(1);
        cnt--;
    }
    return nullptr;
}

void detach_test() {
    pthread_t t;
    int arg = 5;
    pthread_create(&t, NULL, thread3, &arg);

    sleep(1);

    if (pthread_join(t, nullptr) == 0) {
        cout << "pthread wait success" << endl;
    } else {
        cout << "pthread wait failed" << endl;
    }
}

//pthread_attr_setdetachstate 实现线程分离
//int pthread_attr_init(pthread_attr_t *attr);   初始化线程属性结构
//int pthread_attr_destroy(pthread_attr_t *attr);
//int pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate);
//PTHREAD_CREATE_DETACHED（分离态）与 PTHREAD_CREATE_JOINABLE（结合态）。

void *thread4(void *arg) {
    cout << "thread4" << endl;
    return nullptr;
}

void setdetachstate_test() {
    pthread_t t;
    pthread_attr_t attr;
    //初始化结构
    pthread_attr_init(&attr);
    //设置分离态
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&t, &attr, thread4, nullptr);

    if (pthread_join(t, nullptr) == 0) {
        cout << "pthread wait success" << endl;
    } else {
        cout << "pthread wait failed" << endl;
    }
}

//线程的取消
//1．设置线程取消状态
//pthread_cancel()函数向由 thread 指定的线程发送一个取消请求。
//int pthread_setcancelstate(int state, int *oldstate);
void *thread5(void *arg) {
    //设置线程不可取消
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    while (1) {
        cout << "thread5" << endl;
        sleep(1);
    }

    return nullptr;
}

void cancel_test() {
    pthread_t t;

    pthread_create(&t, NULL, thread5, nullptr);

    sleep(5);
    pthread_cancel(t); //取消线程

    pthread_join(t, nullptr);
}

//2．设置线程取消类型
//int pthread_setcanceltype(int type, int *oldtype);
//参数 type 可以被设置为 PTHREAD_CANCEL_DEFERRED，表示线程接收取消操作后，直到运行到“可取消点”后取消。
//type 也可以被设置为 PTHREAD_CANCEL_ASYNCHRONOUS，表示接收取消操作后，立即取消。
void *thread6(void *arg) {
    //设置可取消
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    //表示线程接收取消操作后，直到运行到“可取消点”后取消。
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while (1) {
        cout << "thread6" << endl;
        sleep(1);
    }
    return nullptr;
}

void cancel_type_test() {
    pthread_t t;

    pthread_create(&t, NULL, thread6, nullptr);

    sleep(5);
    pthread_cancel(t);

    pthread_join(t, nullptr);
}

//void pthread_testcancel(void);
//pthread_testcancel()函数用来给当前线程设置一个“可取消点”。
void *thread7(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    //取消点取消
    //pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);
    //直接取消
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while (1) {
        printf("thread7 aa\n");
        pthread_testcancel(); //取消点
        printf("thread7 bb\n");
    }
    return nullptr;
}

void testcancel_test() {
    pthread_t t;

    pthread_create(&t, NULL, thread7, nullptr);

    pthread_cancel(t);

    pthread_join(t, nullptr);
}

//线程同步互斥机制
//线程之间的数据传递。
int g_int1 = 0;
int g_int2 = 0;
int g_cnt = 0;

void *thread8(void *arg) {
    while (1) {
        g_int1 = g_cnt;
        g_int2 = g_cnt;
        g_cnt++;
    }
}

void *thread9(void *arg) {
    while (1) {
        if (g_int1 != g_int2) {
            sleep(1);
            //发生了数据不相等的情况
            cout << "g_int1 = " << g_int1 << " " << "g_int2 = " << g_int2 << endl;
        }
    }
}

void data_test() {
    pthread_t t1, t2;

    pthread_create(&t1, nullptr, thread8, nullptr);
    pthread_create(&t2, nullptr, thread9, nullptr);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
}

//互斥锁的使用
//线程在访问共享的全局变量时，没有按照一定的规则顺序进行访问造成了不可预计的后果。
//针对代码的运行结果分析，其原因就是线程在访问共享资源的过程中被其他线程打断，其他线程也开始访问共享资源导致了数据的不确定性。
//int pthread_mutex_destroy(pthread_mutex_t *mutex);
//int pthread_mutex_init(pthread_mutex_t *restrict mutex,const pthread_mutexattr_t *restrict attr);
//int pthread_mutex_lock(pthread_mutex_t *mutex);
//int pthread_mutex_unlock(pthread_mutex_t *mutex);

pthread_mutex_t lock;

void *thread10(void *arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        g_int1 = g_cnt;
        g_int2 = g_cnt;
        g_cnt++;
        pthread_mutex_unlock(&lock);
    }
}

void *thread11(void *arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        if (g_int1 != g_int2) {
            sleep(1);
            //发生了数据不相等的情况
            cout << "g_int1 = " << g_int1 << " " << "g_int2 = " << g_int2 << endl;
        }
        pthread_mutex_unlock(&lock);
    }
}

void lock_test() {
    pthread_mutex_init(&lock, nullptr);
    pthread_t t1, t2;

    pthread_create(&t1, nullptr, thread10, nullptr);
    pthread_create(&t2, nullptr, thread11, nullptr);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    pthread_mutex_destroy(&lock);
}

//Pthread API 还提供了pthread_mutex_lock()函数的两个版本：pthread_mutex_trylock()和 pthread_mutex_timedlock()。
//int pthread_mutex_trylock(pthread_mutex_t *mutex);
//int pthread_mutex_timedlock(pthread_mutex_t *restrict mutex,const struct timespec *restrict abs_timeout);

//互斥锁的死锁
//（1）在互斥锁默认属性的情况下，在同一个线程中不允许对同一互斥锁连续进行加锁操作。
//因为之前锁处于未解除状态，如果再次对同一个互斥锁进行加锁，那么必然会导致程序无限阻塞等待。
//（2）多个线程对多个互斥锁交叉使用，每一个线程都试图对其他线程所持有的互斥锁进行加锁。
//线程分别持有了对方需要的锁资源，并相互影响，可能会导致程序无限阻塞，就会造成死锁。
//（3）一个持有互斥锁的线程被其他线程取消，其他线程将无法获得该锁，则会造成死锁。
void *thread12(void *arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        g_int1 = g_cnt;
        g_int2 = g_cnt;
        g_cnt++;
        sleep(3);
        pthread_mutex_unlock(&lock);
    }
}

void *thread13(void *arg) {
    while (1) {
        sleep(1); //保证thread12先拿到锁
        pthread_mutex_lock(&lock);
        if (g_int1 != g_int2) {
            sleep(1);
            cout << "g_int1 = " << g_int1 << " " << "g_int2 = " << g_int2 << endl;
        }
        pthread_mutex_unlock(&lock);
    }
}

void deadlock_test() {
    pthread_mutex_init(&lock, nullptr);
    pthread_t t1, t2;

    pthread_create(&t1, nullptr, thread12, nullptr);
    pthread_create(&t2, nullptr, thread13, nullptr);

    sleep(2);
    //并在线程 1 持有互斥锁期间被取消。此时线程 1 使用的互斥锁将无法被获取，造成死锁。
    pthread_cancel(t1);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    pthread_mutex_destroy(&lock);
}

//线程可以设置一个或多个清理函数
//每一个线程都有一个清理函数栈。当线程遭取消时，会沿该栈自顶向下依次执行清理函数。
//当执行完所有的清理函数后，线程终止。pthread_cleanup_push()函数和 pthread_cleanup_pop()函数分别负责向调用线程的清理函数栈添加和移除清理函数。
//void pthread_cleanup_push(void (*routine)(void *),void *arg);
//void pthread_cleanup_pop(int execute);
//如果 execute 为非零（通常使用 1），则执行栈顶的清理处理程序，然后将其从栈中弹出。
//如果 execute 为零（0），则不执行栈顶的清理处理程序，但将其从栈中弹出。
void thread_clean(void *) {
    cout << "清理函数执行" << endl;
    pthread_mutex_unlock(&lock);
}

void *thread14(void *arg) {
    while (1) {
        pthread_mutex_lock(&lock);
        pthread_cleanup_push(thread_clean, nullptr);
            g_int1 = g_cnt;
            g_int2 = g_cnt;
            g_cnt++;
            sleep(3);
        pthread_cleanup_pop(0); //弹出清理函数，免的越加越多
        pthread_mutex_unlock(&lock);
    }
}

void *thread15(void *arg) {
    while (1) {
        sleep(1); //保证thread12先拿到锁
        pthread_mutex_lock(&lock);
        if (g_int1 == g_int2) {
            sleep(1);
            cout << "g_int1 = " << g_int1 << " " << "g_int2 = " << g_int2 << endl;
        }
        pthread_mutex_unlock(&lock);
    }
}

void clean_test() {
    pthread_mutex_init(&lock, nullptr);
    pthread_t t1, t2;

    pthread_create(&t1, nullptr, thread14, nullptr);
    pthread_create(&t2, nullptr, thread15, nullptr);

    sleep(2);
    //并在线程 1 持有互斥锁期间被取消。此时线程 1 使用的互斥锁将无法被获取，造成死锁。
    pthread_cancel(t1);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    pthread_mutex_destroy(&lock);
}

//互斥锁的属性
//初始化互斥锁 pthread_mutex_init()函数中参数 attr 为指定互斥锁的属性
//int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);
//int pthread_mutexattr_init(pthread_mutexattr_t *attr);
//int pthread_mutexattr_getpshared(const pthread_mutexattr_t *restrict attr, int *restrict pshared);
//int pthread_mutexattr_setpshared(pthread_mutexattr_t *attr,int pshared);

//参数 pshared 可以设置为两种情况：
//（1）PTHREAD_PROCESS_PRIVATE，表示互斥锁只能在一个进程内部的两个线程进行互斥（默认情况）；
//（2）PTHREAD_PROCESS_SHARED，互斥锁可用于两个不同进程中的线程进行互斥，使用时需要在共享内存（后续介绍）中分配互斥锁，再为互斥锁指定该属性即可。

//int pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict attr,int *restrict type);
//int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);

//（1）PTHREAD_MUTEX_NORMAL：标准互斥锁
//（2）PTHREAD_MUTEX_ERRORCHECK：检错互斥锁
//（3）PTHREAD_MUTEX_RECURSIVE：递归互斥锁
void mutex_test() {
    pthread_mutex_t mutex;
    //锁的属性
    pthread_mutexattr_t mattr;
    //初始化属性
    pthread_mutexattr_init(&mattr);

    //设置可递归
    pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&mutex, &mattr);

    //上锁
    pthread_mutex_lock(&mutex);
    pthread_mutex_lock(&mutex);

    cout << "mutex lock ok" << endl;

    //上几次锁，就要解几次锁
    pthread_mutex_unlock(&mutex);
    pthread_mutex_unlock(&mutex);

    pthread_mutexattr_destroy(&mattr);
    pthread_mutex_destroy(&mutex);
}

//信号量的使用
//信号量本身代表一种资源，其本质是一个非负的整数计数器，被用来控制对公共资源的访问。换句话说，信号量的核心内容是信号量的值。
//信号量作为一种同步互斥机制，若用于实现互斥时，多线程只需设置一个信号量。
//若用于实现同步时，则需要设置多个信号量，并通过设置不同的信号量的初始值来实现线程的执行顺序。
//int sem_init(sem_t *sem, int pshared, unsigned int value);
//int sem_destroy(sem_t *sem);
//int sem_wait(sem_t *sem);
//int sem_trywait(sem_t * sem);
//int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
//int sem_post(sem_t *sem);   释放信号量的操作
//int sem_getvalue(sem_t *sem, int *sval);   获得当前信号量的值
sem_t sem1, sem2;
char buf[32] = {0};

void *thread16(void *arg) {
    while (1) {
        sem_wait(&sem2); //sem2初始值为1，这里不会阻塞
        fgets(buf, 32, stdin);
        buf[strlen(buf) - 1] = '\0';
        sem_post(&sem1);
    }
}

void *thread17(void *arg) {
    while (1) {
        sem_wait(&sem1); //这里会阻塞，等待用户输入完成
        printf("%s\n", buf);
        sem_post(&sem2);
    }
}

void sem_test() {
    pthread_t t1, t2;
    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 1);

    pthread_create(&t1, nullptr, thread16, nullptr);
    pthread_create(&t2, nullptr, thread17, nullptr);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);

    sem_destroy(&sem1);
    sem_destroy(&sem2);
}

//条件变量的使用
//条件变量的工作原理很简单，即让当前不需要访问共享资源的线程进行阻塞等待（睡眠），如果某一时刻就共享资源的状态改变需要某一个线程处理，那么则可以通知该线程进行处理（唤醒）。
//int pthread_cond_destroy(pthread_cond_t *cond);
//int pthread_cond_init(pthread_cond_t *restrict cond, const pthread_condattr_t *restrict attr);
//int pthread_cond_broadcast(pthread_cond_t *cond);   唤醒当前条件变量所指定的所有阻塞等待的线程。
//int pthread_cond_signal(pthread_cond_t *cond);  发送信号给至少一个处于阻塞等待的线程，使其脱离阻塞状态，继续执行
//int pthread_cond_timedwait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex, const struct timespec *restrict abstime);
//int pthread_cond_wait(pthread_cond_t *restrict cond, pthread_mutex_t *restrict mutex);
//pthread_cond_wait()函数一旦实现阻塞，使线程进入睡眠之后，函数自身会将之前线程已经持有的互斥锁自动释放。 不同于唤醒操作，睡眠操作必须要进行加锁处理。
pthread_cond_t cond;
pthread_mutex_t lck;

void *thread18(void *arg) {
    while (1) {
        fgets(buf, 32, stdin);
        buf[strlen(buf) - 1] = '\0';
        pthread_cond_signal(&cond);
    }
}

void *thread19(void *arg) {
    while (1) {
        pthread_mutex_lock(&lck);
        /*线程执行阻塞，此时自动执行解锁，当线程收到唤醒信号，函数立即返回，此时在进入临界区之前，再次自动加锁*/
        pthread_cond_wait(&cond, &lck); //之前必需加锁
        printf("thread19 %s\n", buf); //临界区
        sleep(1);
        pthread_mutex_unlock(&lck);
    }
}

void *thread20(void *arg) {
    while (1) {
        pthread_mutex_lock(&lck);
        pthread_cond_wait(&cond, &lck); //之前必需加锁
        printf("thread20 %s\n", buf); //临界区
        sleep(1);
        pthread_mutex_unlock(&lck);
    }
}

void cond_test() {
    pthread_t t1, t2, t3;

    pthread_cond_init(&cond, nullptr);
    pthread_mutex_init(&lck, nullptr);

    pthread_create(&t1, nullptr, thread18, nullptr);
    pthread_create(&t2, nullptr, thread19, nullptr);
    pthread_create(&t3, nullptr, thread20, nullptr);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    pthread_join(t3, nullptr);

    pthread_mutex_destroy(&lck);
    pthread_cond_destroy(&cond);
}

pthread_cond_t cond2;
pthread_mutex_t lck2;
int g_cnt2 = 0;

void *thread21(void *arg) {
    while (1) {
        cout << "g_cnt2 = " << ++g_cnt2 << endl;
        sleep(1);
        pthread_mutex_lock(&lck2);
        strcpy(buf, "hello");
        pthread_mutex_unlock(&lck2);
        pthread_cond_signal(&cond2); //发送信号
    }
}

void *thread22(void *arg) {
    while (1) {
        pthread_mutex_lock(&lck2);
        pthread_cond_wait(&cond2, &lck2); //之前必需加锁
        printf("thread22 %s\n", buf); //临界区
        sleep(1);
        pthread_mutex_unlock(&lck2);
    }
}

void *thread23(void *arg) {
    while (1) {
        pthread_mutex_lock(&lck2);
        pthread_cond_wait(&cond2, &lck2); //之前必需加锁
        printf("thread23 %s\n", buf); //临界区
        sleep(1);
        pthread_mutex_unlock(&lck2);
    }
}

void cond_test2() {
    pthread_t t1, t2, t3;

    pthread_cond_init(&cond2, nullptr);
    pthread_mutex_init(&lck2, nullptr);

    pthread_create(&t1, nullptr, thread21, nullptr);
    pthread_create(&t2, nullptr, thread22, nullptr);
    pthread_create(&t3, nullptr, thread23, nullptr);

    pthread_join(t1, nullptr);
    pthread_join(t2, nullptr);
    pthread_join(t3, nullptr);

    pthread_mutex_destroy(&lck2);
    pthread_cond_destroy(&cond2);
}

//线程池
//把一堆开辟好的线程放在一个池子里统一管理，就是一个线程池。
//（1）线程管理器：创建并管理线程池。
//（2）工作线程：线程池中实际执行任务的线程。在初始化线程池时，将预先创建好固定数目的线程在池中，这些初始化的线程一般处于空闲状态。
//（3）任务接口：每个任务必须实现的接口，当线程池的任务队列中有可执行任务时，被空闲的 工作线程调去执行，把任务抽象成接口，可以做到线程池与具体的任务无关。
//（4）任务队列：用来存放没有处理的任务，提供一种缓冲机制，实现这种结构的方法有很多，常使用队列，主要运用先进先出的原理。

//线程池的实现
//（1）用户程序向任务队列中添加任务。
//（2）创建线程池，线程睡眠，处于空闲状态。
//（3）唤醒线程，线程池中的线程执行函数取出任务队列中的任务。
//（4）执行任务中的调用函数，完成工作。
//（5）线程池任务执行完判断，如果没有程序调用，线程继续睡眠。
//（6）调用销毁函数对线程池进行销毁。
class Worker {
public:
    void *(*process)(void *arg);

    void *arg;
};

class ThreadPool {
public:
    ThreadPool(int thread_num = 10) : max_thread_num(thread_num) {
        pool_init(thread_num);
    }

    void *thread_routine() {
        while (1) {
            pthread_mutex_lock(&queue_lock);

            //如果等待队列为 0 并且不摧毁线程池，则线程处于阻塞状态
            while (cur_queue_size == 0 && !shutdown) {
                pthread_cond_wait(&queue_ready, &queue_lock);
            }

            //线程池如果摧毁
            if (shutdown) {
                pthread_mutex_unlock(&queue_lock);
                pthread_exit(nullptr);
            }

            cur_queue_size--;
            Worker *work = workers.front();
            workers.pop_front();

            pthread_mutex_unlock(&queue_lock);

            //调用回调函数，执行任务
            work->process(work->arg);

            delete work;
        }
    }

    void pool_init(int thread_num) {
        pthread_mutex_init(&queue_lock, nullptr);
        pthread_cond_init(&queue_ready, nullptr);

        max_thread_num = thread_num;
        cur_queue_size = 0;

        shutdown = 0;

        thread = new pthread_t[thread_num];

        for (int i = 0; i < max_thread_num; i++) {
            pthread_create(&thread[i], nullptr, ThreadPool::threadWrapper, this);
        }
    }

    void pool_add_worker(void *(*process)(void *arg), void *arg) {
        Worker *work = new Worker;
        work->process = process;
        work->arg = arg;

        pthread_mutex_lock(&queue_lock);

        workers.push_back(work);
        cur_queue_size++;

        pthread_mutex_unlock(&queue_lock);

        //等待队列中有任务，唤醒一个等待线程
        pthread_cond_signal(&queue_ready);
    }

    ~ThreadPool() {
        if (shutdown) {
            return;
        }

        shutdown = 1;

        //唤醒所有等待的线程，线程池要摧毁了
        pthread_cond_broadcast(&queue_ready);

        for (int i = 0; i < max_thread_num; i++) {
            pthread_join(thread[i], nullptr);
        }

        delete [] thread;

        cout << "workers.size()=" << workers.size() << endl;

        pthread_mutex_destroy(&queue_lock);
        pthread_cond_destroy(&queue_ready);
    }

private:
    static void *threadWrapper(void *obj) {
        ThreadPool *myClass = static_cast<ThreadPool *>(obj);
        return myClass->thread_routine();
    }

    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;

    list<Worker *> workers;

    int shutdown; //是否摧毁线程池
    pthread_t *thread;

    int max_thread_num; //线程池中允许活动线程的数量
    int cur_queue_size; //当前等待队列的任务数量
};

void *myprocess(void *arg) {
    printf("threadid is %x working on task %d\n", pthread_self(), *(int *) arg);
    return nullptr;
}

void threadpool_test() {
    ThreadPool pool(10);
    sleep(1);
    int *num = new int[10];
    for (int i = 0; i < 10; i++) {
        num[i] = i;
        pool.pool_add_worker(myprocess, &num[i]);
    }
    sleep(5);
    delete[] num;
}

int main() {
    //thread_test();
    //detach_test();
    //setdetachstate_test();
    //cancel_test();
    //cancel_type_test();
    //testcancel_test();
    //data_test();
    //lock_test();
    //deadlock_test();
    //clean_test();
    //mutex_test();
    //sem_test();
    //cond_test();
    //cond_test2();
    threadpool_test();
    return 0;
}
