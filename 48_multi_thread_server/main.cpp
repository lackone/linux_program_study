#include <cstring>
#include <iostream>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//引入线程的背景
//为了分时使用CPU，需要上下文切换。上下文切换需要很长时间。
//1、线程的创建和上下文切换比进程的创建和上下文切换更快。
//2、线程间交换数据时无需特殊技术。

//线程和进程的差异
//每个进程都拥有独立空间

//1、上下文切换时不需要切换数据区和堆
//2、可以利用数据区和堆交换数据

//进程：在操作系统构成单独执行流的单位
//线程：在进程构成单独执行流的单位

//线程创建及运行
//int pthread_create(pthread_t * thread, const pthread_attr_t * attr, void *(*start_routine)(void*), void* arg);
void *thread_func(void *arg) {
    int num = *(int *) arg;
    for (int i = 0; i < num; i++) {
        sleep(1);
        printf("thread running\n");
    }
    return NULL;
}

void thread_test() {
    pthread_t tid;
    int n = 5;
    pthread_create(&tid, NULL, thread_func, &n);
    sleep(10);
}

//int pthread_join(pthread_t thread, void **status);
//调用该函数的进程将进入等待状态，直到线程的终止为止。
void *thread_func2(void *arg) {
    int num = *(int *) arg;
    for (int i = 0; i < num; i++) {
        sleep(1);
        printf("thread running\n");
    }
    return (void *) 666;
}

void thread_test2() {
    pthread_t tid;
    int n = 5;
    pthread_create(&tid, NULL, thread_func2, &n);
    void *ret;
    pthread_join(tid, &ret);
    printf("ret = %d\n", (void *) ret);
}

//线程安全函数
//非线程安全函数

//gethostbyname  gethostbyname_r
//线程安全函数的名称后缀通常为_r
int sum = 0;

void *thread_sum(void *arg) {
    int start = ((int *) (arg))[0];
    int end = ((int *) (arg))[1];

    while (start <= end) {
        sum += start;
        start++;
    }
    return NULL;
}

void thread_test3() {
    pthread_t tid1, tid2;
    int range1[] = {1, 5};
    int range2[] = {6, 10};

    pthread_create(&tid1, NULL, thread_sum, (void *) range1);
    pthread_create(&tid2, NULL, thread_sum, (void *) range2);

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    printf("sum = %d\n", sum);
}

int num = 0;

void *thread_add(void *arg) {
    for (int i = 0; i < 1000; i++) {
        //线程同时访问变量
        num++;
    }
    return NULL;
}

void *thread_sub(void *arg) {
    for (int i = 0; i < 1000; i++) {
        //线程同时访问变量
        num--;
    }
    return NULL;
}

void thread_test4() {
    pthread_t tid[100];
    for (int i = 0; i < 100; i++) {
        if (i % 2) {
            pthread_create(&tid[i], NULL, thread_add, NULL);
        } else {
            pthread_create(&tid[i], NULL, thread_sub, NULL);
        }
    }
    for (int i = 0; i < 100; i++) {
        pthread_join(tid[i], NULL);
    }
    //此处的数据会有问题
    printf("num = %d\n", num);
}

//线程同步
//线程同步用于解决线程访问顺序引发的问题

//互斥量
//表示不允许多个线程同时访问
//int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
//int pthread_mutex_destroy(pthread_mutex_t *mutex);
//int pthread_mutex_lock(pthread_mutex_t *mutex);
//int pthread_mutex_unlock(pthread_mutex_t *mutex);
int num2 = 0;
pthread_mutex_t mutex;

void *thread_add2(void *arg) {
    //最大限度的减少调用lock和unlock的次数
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < 1000; i++) {
        //线程同时访问变量
        //pthread_mutex_lock(&mutex);
        num2++;
        //pthread_mutex_unlock(&mutex);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *thread_sub2(void *arg) {
    for (int i = 0; i < 1000; i++) {
        //线程同时访问变量
        pthread_mutex_lock(&mutex);
        num2--;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void thread_test5() {
    pthread_t tid[100];

    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < 100; i++) {
        if (i % 2) {
            pthread_create(&tid[i], NULL, thread_add2, NULL);
        } else {
            pthread_create(&tid[i], NULL, thread_sub2, NULL);
        }
    }
    for (int i = 0; i < 100; i++) {
        pthread_join(tid[i], NULL);
    }
    //此处的数据会有问题
    printf("num2 = %d\n", num2);

    pthread_mutex_destroy(&mutex);
}

//信号量
//int sem_init(sem_t *sem, int pshared, unsigned int value);
//int sem_destroy(sem_t *sem);
//int sem_post(sem_t *sem);  增1
//int sem_wait(sem_t *sem);  减1
sem_t sem1;
sem_t sem2;
int num3;

void *read(void *arg) {
    for (int i = 0; i < 5; i++) {
        printf("input num:");
        sem_wait(&sem2);
        scanf("%d", &num3);
        sem_post(&sem1);
    }
    return NULL;
}

void *accu(void *arg) {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sem_wait(&sem1);
        sum += num3;
        sem_post(&sem2);
    }
    printf("sum = %d\n", sum);
    return NULL;
}

void sem_test() {
    pthread_t t1, t2;
    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 1);

    pthread_create(&t1, NULL, read, NULL);
    pthread_create(&t2, NULL, accu, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sem_destroy(&sem1);
    sem_destroy(&sem2);
}

//销毁线程的3种方法
//调用 pthread_join       等待线程终止，让调用该函数的线程进入阻塞状态
//调用 pthread_detach

//多线程并发服务器端的实现
pthread_mutex_t chat_mutex;
int socks[1024];
int sock_index = 0;

void send_msg(const char *msg, int len) {
    pthread_mutex_lock(&chat_mutex);
    for (int i = 0; i < sock_index; i++) {
        write(socks[i], msg, len);
    }
    pthread_mutex_unlock(&chat_mutex);
}

void *clt_sock_handler(void *arg) {
    int clt_sock = *((int *) arg);
    int len;
    char buf[256] = {0};

    while ((len = read(clt_sock, buf, sizeof(buf))) > 0) {
        send_msg(buf, len);
    }

    //移除客户端
    pthread_mutex_lock(&chat_mutex);
    for (int i = 0; i < sock_index; i++) {
        if (clt_sock == socks[i]) {
            while (i++ < sock_index - 1) {
                socks[i] = socks[i + 1];
            }
            break;
        }
    }
    sock_index--;
    pthread_mutex_unlock(&chat_mutex);

    close(clt_sock);
    return NULL;
}

void chat_server(int port) {
    sockaddr_in srv_addr, clt_addr;
    socklen_t clt_addr_len = sizeof(clt_addr);

    //初始化锁
    pthread_mutex_init(&chat_mutex, NULL);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(port);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *) &srv_addr, sizeof(srv_addr));
    listen(sock, 5);

    pthread_t tid;
    int clt_sock;

    while (1) {
        clt_sock = accept(sock, (struct sockaddr *) &clt_addr, &clt_addr_len);

        pthread_mutex_lock(&chat_mutex);
        socks[sock_index++] = clt_sock;
        pthread_mutex_unlock(&chat_mutex);

        pthread_create(&tid, NULL, clt_sock_handler, (void *) &clt_sock);
        //将线程设置为脱离状态
        pthread_detach(tid);

        printf("client connect %s %d\n", inet_ntoa(clt_addr.sin_addr), ntohs(clt_addr.sin_port));
    }

    close(sock);
}

char client_name[32];

void *read_msg(void *arg) {
    int sock = *((int *) arg);
    char buf[256] = {0};
    int len;
    while (1) {
        len = read(sock, buf, sizeof(buf));
        if (len == -1) {
            break;
        }
        buf[len] = '\0';
        printf("%s\n", buf);
    }
    return NULL;
}

void *write_msg(void *arg) {
    int sock = *((int *) arg);
    char buf[256] = {0};
    char msg[32 + 256 + 1] = {0};
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
        }
        sprintf(msg, "%s %s", client_name, buf);
        write(sock, msg, strlen(msg));
    }
    close(sock);
    return NULL;
}

void chat_client(int port, const char *name) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    connect(sock, (struct sockaddr *) &addr, sizeof(addr));

    sprintf(client_name, "[%s]", name);

    pthread_t read_t, write_t;
    pthread_create(&read_t, NULL, read_msg, (void *) &sock);
    pthread_create(&write_t, NULL, write_msg, (void *) &sock);
    pthread_join(read_t, NULL);
    pthread_join(write_t, NULL);
    close(sock);
}

int main(int argc, char *argv[]) {
    //thread_test();
    //thread_test2();
    //thread_test3();
    //thread_test4();
    //thread_test5();
    //sem_test();
    if (strcmp(argv[1], "server") == 0) {
        chat_server(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        chat_client(atoi(argv[2]), argv[3]);
    }
    return 0;
}
