#include <cstring>
#include <iostream>
#include <signal.h>
#include <sys/msg.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
using namespace std;

//消息队列
//消息队列就是一些消息的列表，或者说是一些消息组成的队列。
//key_t ftok(const char *pathname, int proj_id);

//（1）创建或打开队列
//int msgget(key_t key, int msgflg);
//（2）发送消息
//int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
//（3）消息接收
//ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
//如果msgtyp为0，则接收队列中的第一个消息。
//如果msgtyp大于0，则接收第一个类型等于msgtyp的消息。
//如果msgtyp小于0，则接收类型小于或等于绝对值|msgtyp|的第一个消息。
//（4）消息的控制
//int msgctl(int msqid, int cmd, struct msqid_ds *buf);
struct mymsg {
    long mtype;
    int a;
    char b;
    char buf[256];
};

void msg_test() {
    //创建 key 值
    key_t key = ftok(".", 'a');
    //创建或打开消息队列
    int msgid = msgget(key, IPC_CREAT | IPC_EXCL | 0664);
    if (msgid < 0) {
        if (errno != EEXIST) {
            perror("msgget");
            return;
        }
        msgid = msgget(key, 0664);
    }

    //封装消息到结构体
    struct mymsg msg;
    msg.mtype = 111;
    msg.a = 22;
    msg.b = 'b';
    strcpy(msg.buf, "hello");
    //发送消息
    msgsnd(msgid, (void *) &msg, sizeof(struct mymsg) - sizeof(long), 0);

    //调用 Shell 命令查看系统中的消息队列
    system("ipcs -q");
}

void msg_get_test() {
    //创建 key 值
    key_t key = ftok(".", 'a');
    //创建或打开消息队列
    int msgid = msgget(key, IPC_CREAT | IPC_EXCL | 0664);
    if (msgid < 0) {
        if (errno != EEXIST) {
            perror("msgget");
            return;
        }
        msgid = msgget(key, 0664);
    }

    struct mymsg msg;
    msgrcv(msgid, &msg, sizeof(struct mymsg) - sizeof(long), 0, 0);

    cout << msg.mtype << endl;
    cout << msg.a << endl;
    cout << msg.b << endl;
    cout << msg.buf << endl;

    system("ipcs -q");
}

//实验将实现两个终端的信息交互，类似于聊天。 在一个终端中输入，信息可以实时显示到另一个终端，反之 同理。
//子进程发送消息，父进程接收消息。

struct mymsgbuf {
    long mtype;
    char buf[256];
};

#define MSG_TYPE1 111
#define MSG_TYPE2 222

void msg_queue_test1() {
    key_t key = ftok(".", 'a');
    struct mymsgbuf send, recv;
    int msgid = msgget(key, IPC_CREAT | IPC_EXCL | 0664);
    if (msgid < 0) {
        if (errno != EEXIST) {
            perror("msgget");
            return;
        }
        msgid = msgget(key, 0664);
    }
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        //子进程读取终端数据并发送
        while (1) {
            send.mtype = MSG_TYPE1;
            fgets(send.buf, sizeof(send.buf), stdin);
            send.buf[strlen(send.buf) - 1] = '\0';

            //发送消息
            msgsnd(msgid, &send, sizeof(struct mymsgbuf) - sizeof(long), 0);

            if (strncmp(send.buf, "exit", 4) == 0) {
                kill(getppid(), SIGKILL);
                break;
            }
        }
    } else {
        while (1) {
            msgrcv(msgid, &recv, sizeof(struct mymsgbuf) - sizeof(long), MSG_TYPE2, 0);

            if (strncmp(recv.buf, "exit", 4) == 0) {
                kill(pid, SIGKILL);
                goto ERR;
            }

            printf("%s\n", recv.buf);
        }
    }
ERR:
    msgctl(msgid, IPC_RMID, NULL);
}

void msg_queue_test2() {
    key_t key = ftok(".", 'a');
    struct mymsgbuf send, recv;
    int msgid = msgget(key, IPC_CREAT | IPC_EXCL | 0664);
    if (msgid < 0) {
        if (errno != EEXIST) {
            perror("msgget");
            return;
        }
        msgid = msgget(key, 0664);
    }
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        //子进程读取终端数据并发送
        while (1) {
            send.mtype = MSG_TYPE2;
            fgets(send.buf, sizeof(send.buf), stdin);
            send.buf[strlen(send.buf) - 1] = '\0';

            //发送消息
            msgsnd(msgid, &send, sizeof(struct mymsgbuf) - sizeof(long), 0);

            if (strncmp(send.buf, "exit", 4) == 0) {
                kill(getppid(), SIGKILL);
                break;
            }
        }
    } else {
        while (1) {
            msgrcv(msgid, &recv, sizeof(struct mymsgbuf) - sizeof(long), MSG_TYPE1, 0);

            if (strncmp(recv.buf, "exit", 4) == 0) {
                kill(pid, SIGKILL);
                goto ERR;
            }

            printf("%s\n", recv.buf);
        }
    }
ERR:
    msgctl(msgid, IPC_RMID, NULL);
}

//共享内存
//共享内存是一种最为高效的进程间通信方式。因为进程可以直接读写内存，而无须创建任何形式的载体即可完成数据的传递。
//（1）创建或打开共享内存段。
//int shmget(key_t key, size_t size, int shmflg);
//（2）将共享内存段映射到进程的虚拟地址空间上。
//void *shmat(int shmid, const void *shmaddr, int shmflg);
//（3）将共享内存段与进程的虚拟地址空间映射关系断开。
//int shmdt(const void *shmaddr);
//（4）共享内存放入控制。
//int shmctl(int shmid, int cmd, struct shmid_ds *buf);

struct shmbuf {
    int a;
    char b;
};

//写共享内存
void shm_test() {
    key_t key = ftok(".", 'q');
    int shmid = shmget(key, 1024, IPC_CREAT | IPC_EXCL | 0664);
    if (shmid < 0) {
        if (errno != EEXIST) {
            perror("shmget");
            return;
        }
        shmid = shmget(key, 1024, 0664);
    }
    //用自定义的结构体指针接收函数的返回值
    struct shmbuf *msg = nullptr;
    msg = (struct shmbuf *) shmat(shmid, NULL, 0);
    printf("shm %p\n", msg);
    //向虚拟内存写入数据
    msg->a = 111;
    msg->b = 'b';
    //断开映射
    shmdt(msg);

    system("ipcs -m");
}

//读共享内存
void shm_test2() {
    key_t key = ftok(".", 'q');
    int shmid = shmget(key, 1024, IPC_CREAT | IPC_EXCL | 0664);
    if (shmid < 0) {
        if (errno != EEXIST) {
            perror("shmget");
            return;
        }
        shmid = shmget(key, 1024, 0664);
    }

    //从虚拟内存中读取数据
    struct shmbuf *msg = nullptr;
    msg = (struct shmbuf *) shmat(shmid, NULL, 0);
    printf("shm %p\n", msg);
    printf("%d %c", msg->a, msg->b);

    shmdt(msg);

    //删除共享内存
    shmctl(shmid, IPC_RMID, NULL);

    system("ipcs -m");
}

//信号灯
//共享内存作为进程间最高效的通信机制，其缺陷也十分明显。 为了保证进程在访问同一内存区域而不会产生竞态，共享内存需要与同步互斥机制配合使用。
//信号灯其操作与信号量基本类似，不同的是信号灯可以操作多个信号量。
//对每个信号量的核心操作为 PV 操作，P 操作即申请信号量，如果信号量的值大于0则申请成功，信号量的值减1，
//如果信号量的值为0则申请阻塞；V 操作即释放信号量，如果释放成功，则信号量的值加1。

//1．创建或打开信号量集
//int semget(key_t key, int nsems, int semflg);
//2．信号量集控制（信号量初始化、信号量删除等）
//int semctl(int semid, int semnum, int cmd, ...);
//信号量集合中的信号量的编号从 0 开始，与数组元素类似。
//3．PV 操作（申请信号量、释放信号量）
//int semop(int semid, struct sembuf *sops, unsigned nsops);

union mysembuf {
    int val;
};

void sem_test() {
    key_t key = ftok(".", 'q');

    int semid = semget(key, 2, IPC_CREAT | IPC_EXCL | 0664);
    if (semid < 0) {
        if (errno != EEXIST) {
            perror("semget");
            return;
        }
        semid = semget(key, 2, 0664);
    }
    //初始化信号量的值,第一个信号量的编号为 0,第二个信号量的编号为 1
    union mysembuf sem;
    sembuf buf;

    sem.val = 1;
    semctl(semid, 0, SETVAL, sem);

    sem.val = 1;
    semctl(semid, 1, SETVAL, sem);

    //对编号为 0 的信号量执行申请操作
    buf.sem_num = 0;
    buf.sem_op = -1; //申请
    buf.sem_flg = 0;
    semop(semid, &buf, 1);

    //对编号为 1 的信号量执行释放操作
    buf.sem_num = 1;
    buf.sem_op = 1; //释放
    buf.sem_flg = 0;
    semop(semid, &buf, 1);

    int ret = semctl(semid, 0, GETVAL);
    printf("%d\n", ret);

    ret = semctl(semid, 1, GETVAL);
    printf("%d\n", ret);

    //删除信号量
    semctl(semid, 0, IPC_RMID);
    semctl(semid, 1, IPC_RMID);
}

//共享内存
struct myshmbuf {
    char buf[256];
};

union mysemun {
    int val;
};

void sem_test2() {
    key_t key = ftok(".", 'q');
    int shmid = shmget(key, 1024, IPC_CREAT | IPC_EXCL | 0664);
    if (shmid < 0) {
        if (errno != EEXIST) {
            perror("shmget");
            return;
        }
        shmid = shmget(key, 1024, 0664);
    }
    struct myshmbuf *msg = nullptr;
    msg = (struct myshmbuf *) shmat(shmid, NULL, 0);
    printf("shm %p\n", msg);

    //创建2个信号量
    int semid = semget(key, 2, IPC_CREAT | IPC_EXCL | 0664);
    if (semid < 0) {
        if (errno != EEXIST) {
            perror("semget");
            return;
        }
        semid = semget(key, 2, 0664);
    } else {
        //初始化信号量的值
        union mysemun sem;
        sem.val = 0;
        semctl(semid, 0, SETVAL, sem);
        sem.val = 1;
        semctl(semid, 1, SETVAL, sem);
    }

    sembuf buf;

    while (1) {
        //申请写操作信号量
        buf.sem_num = 1;
        buf.sem_op = -1;
        buf.sem_flg = 0;
        semop(semid, &buf, 1);

        fgets(msg->buf, 256, stdin);
        msg->buf[strlen(msg->buf) - 1] = '\0';

        //释放读信号量
        buf.sem_num = 0;
        buf.sem_op = 1;
        buf.sem_flg = 0;
        semop(semid, &buf, 1);

        if (strncmp(msg->buf, "exit", 4) == 0) {
            goto ERR;
        }
    }
ERR:
    shmdt(msg);
}

void sem_test3() {
    key_t key = ftok(".", 'q');
    int shmid = shmget(key, 1024, IPC_CREAT | IPC_EXCL | 0664);
    if (shmid < 0) {
        if (errno != EEXIST) {
            perror("shmget");
            return;
        }
        shmid = shmget(key, 1024, 0664);
    }
    struct myshmbuf *msg = nullptr;
    msg = (struct myshmbuf *) shmat(shmid, NULL, 0);
    printf("shm %p\n", msg);

    //创建2个信号量
    int semid = semget(key, 2, IPC_CREAT | IPC_EXCL | 0664);
    if (semid < 0) {
        if (errno != EEXIST) {
            perror("semget");
            return;
        }
        semid = semget(key, 2, 0664);
    } else {
        //初始化信号量的值
        union mysemun sem;
        sem.val = 0;
        semctl(semid, 0, SETVAL, sem);
        sem.val = 1;
        semctl(semid, 1, SETVAL, sem);
    }

    sembuf buf;

    while (1) {
        //申请读操作信号量
        buf.sem_num = 0;
        buf.sem_op = -1;
        buf.sem_flg = 0;
        semop(semid, &buf, 1);

        if (strncmp(msg->buf, "exit", 4) == 0) {
            goto ERR;
        }
        printf("%s\n", msg->buf);

        //释放写信号量
        buf.sem_num = 1;
        buf.sem_op = 1;
        buf.sem_flg = 0;
        semop(semid, &buf, 1);
    }
ERR:
    shmdt(msg);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID, NULL);
    semctl(semid, 1, IPC_RMID, NULL);
}

int main(int argc, char *argv[]) {
    //msg_test();
    //msg_get_test();
    //if (strcmp(argv[1], "1") == 0) {
    //    msg_queue_test1();
    //} else if (strcmp(argv[1], "2") == 0) {
    //    msg_queue_test2();
    //}
    //shm_test();
    //shm_test2();
    //sem_test();
    if (strcmp(argv[1], "2") == 0) {
        sem_test2();
    } else if (strcmp(argv[1], "3") == 0) {
        sem_test3();
    }
    return 0;
}
