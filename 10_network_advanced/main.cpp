#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/ethernet.h> //支持结构体 struct ether_header
//#include <net/if_arp.h> //支持结构体 struct arphdr
using namespace std;

//网络超时检测
//setsockopt()函数实现超时检测
//int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
//int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);

//第三个参数 optname 设置为 SO_RCVTIMEO 或 SO_SNDTIMEO 时，可以实现超时检测。
void timeout_test() {
    struct timeval t = {5, 0};

    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    bind(fd, (struct sockaddr *) &addr, addr_len);

    //接收数据前设置了 5s 的数据接收超时
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));

    int bytes;
    char buf[1024] = {0};

    while (1) {
        if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &client, &addr_len)) < 0) {
            perror("recvfrom error");
        } else {
            printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            if (strncmp(buf, "exit", 4) == 0) {
                printf("server exit\n");
                break;
            }

            buf[bytes] = '\0';
            strcat(buf, "-server");

            if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &client, addr_len) < 0) {
                perror("sendto error");
            }
        }
    }

    close(fd);
}

//select()函数实现超时检测
//使用 select()函数与 setsockopt()函数实现超时检测很类似，因为 select()函数本身就带有超时检测功能。
void select_timeout_test() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    bind(fd, (struct sockaddr *) &addr, addr_len);

    int ret;
    int max_fd;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    max_fd = fd;
    struct timeval t = {5, 0};
    char buf[1024] = {0};
    int bytes;

    while (1) {
        t = {5, 0};

        if ((ret = select(max_fd + 1, &readfds, NULL, NULL, &t)) < 0) {
            perror("select error");
        } else if (ret == 0) {
            //时间到达之前，没有文件描述符准备就绪，那么返回 0。
            perror("select timeout");
        } else {
            if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &client, &addr_len)) < 0) {
                perror("recvfrom error");
            } else {
                printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

                if (strncmp(buf, "exit", 4) == 0) {
                    printf("server exit\n");
                    break;
                }

                buf[bytes] = '\0';
                strcat(buf, "-server");

                if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &client, addr_len) < 0) {
                    perror("sendto error");
                }
            }
        }
    }

    close(fd);
}

//定时器超时检测
//利用定时器信号 SIGALRM，可以在程序中创建一个闹钟。当到达目标时间之后，指定的信号处理函数被执行。
void handler(int sig) {
    if (sig == SIGALRM) {
        printf("超时了\n");
    }
}

void signal_timeout_test() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(6666);

    bind(fd, (struct sockaddr *) &addr, addr_len);

    struct sigaction act;
    //消除之前的信号处理，设置当前的信号处理为空，类似于初始化
    sigaction(SIGALRM, NULL, &act);
    //指定信号处理函数，信号到来时，执行该函数
    act.sa_handler = handler;
    //清除自启动标志，即取消自启动
    act.sa_flags &= ~SA_RESTART;
    //设置本次的信号处理
    sigaction(SIGALRM, &act, NULL);

    int bytes;
    char buf[1024] = {0};

    while (1) {
        alarm(5);

        if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &client, &addr_len)) < 0) {
            perror("recvfrom error");
        } else {
            printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            if (strncmp(buf, "exit", 4) == 0) {
                printf("server exit\n");
                break;
            }

            buf[bytes] = '\0';
            strcat(buf, "-server");

            if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &client, addr_len) < 0) {
                perror("sendto error");
            }
        }

        alarm(0);
    }

    close(fd);
}

//广播
//网络信息传输主要有 4 种方式：单播、任播、组播、广播。
//广播（与组播）可以为应用程序提供两种服务，包括数据分组发送至多个目的地，以及通过客户端请求发现服务器。
//发送到多个目的地，指的是应用程序将信息发送至多个收件方。例如，邮件或新闻分发给多个收件方。

//广播地址
//以 C 类网段 192.168.1.x 为例，其中最小的地址 192.168.1.0代表该网段，而最大的地址192.168.1.255则是该网段中的广播地址。
//当向这个地址发送数据包时，该网段中所有的主机都会接收并处理。

//广播的发送与接收
//（1）创建 UDP 套接字。
//（2）指定目标地址和端口（填充广播信息结构体）。
//（3）设置套接字选项允许发送广播包。
//（4）发送广播消息。
void broadcast_server_test() {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("192.168.1.255");
    addr.sin_port = htons(6666);

    //设置为允许发送广播权限
    int on = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));

    char buf[1024] = {0};

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &addr, addr_len) < 0) {
            perror("sendto error");
        }
    }

    close(fd);
}

//广播包接收的流程如下。
//（1）创建 UDP 套接字。
//（2）填充广播信息结构体（指定地址和端口）。
//（3）绑定信息信息结构体。
//（4）接收广播信息。
void broadcast_client_test() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(6666);

    //绑定了，只有一个端能收到消息
    bind(fd, (struct sockaddr *) &addr, addr_len);

    int bytes;
    char buf[1024] = {0};

    while (1) {
        if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &client, &addr_len)) < 0) {
            perror("recvfrom error");
        } else {
            buf[bytes] = '\0';

            printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            printf("broadcast %s\n", buf);
        }
    }

    close(fd);
}

//组播
//可以只向特定的一部分接收方发送信息，这被称为组播（又称为多播）。当发送组播数据包时，只有加入指定多播组的主机数据链路层才会处理，其他主机在数据链路层会直接丢掉收到的数据包。

//组播地址
//A类 IP 地址的范围为 1.0.0.1到127.255.255.254；
//B类 IP 地址范围为128.0.0.1到191.255.255.254；
//C类 IP 地址范围为192.0.0.1到223.255.255.254；
//D类 IP 地址范围为224.0.0.1到239.255.255.254。
//E 类 IP 地址保留。D 类地址又被称为组播地址。每一个组播地址代表一个多播组。

//组播的发送与接收
//（1）创建 UDP 套接字。
//（2）指定目标地址和端口。
//（3）发送数据包。
void multicast_server_test() {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    ////224.x.x.x - 239.x.x.x
    addr.sin_addr.s_addr = inet_addr("224.1.1.1");
    addr.sin_port = htons(6666);

    char buf[1024] = {0};

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &addr, addr_len) < 0) {
            perror("sendto error");
        }
    }
    close(fd);
}

//组播包接收流程如下。
//（1）创建 UDP 套接字。
//（2）绑定地址和端口
//（3）加入多播组。
//（4）接收数据包
void multicast_client_test() {
    sockaddr_in addr, client;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    ////224.x.x.x - 239.x.x.x
    addr.sin_addr.s_addr = inet_addr("224.1.1.1");
    addr.sin_port = htons(6666);

    bind(fd, (struct sockaddr *) &addr, addr_len);

    //加入多播组，允许数据链路层处理指定数据包
    struct ip_mreq mreq;
    //添加多播组 IP
    mreq.imr_multiaddr.s_addr = inet_addr("224.1.1.1");
    //添加一个将要添加到多播组的 IP 地址
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));

    int bytes;
    char buf[1024] = {0};

    while (1) {
        if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &client, &addr_len)) < 0) {
            perror("recvfrom error");
        } else {
            buf[bytes] = '\0';

            printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            printf("multicast %s\n", buf);
        }
    }
}

//UNIX 域套接字
//流式套接字（SOCK_STREAM）、数据报套接字（SOCK_DGRAM）、 原始套接字（SOCK_RAW）。
//本地通信中，也可以使用流式套接字与数据报套接字来实现。
//通常把用于本地通信的套接字称为 UNIX 域套接字。
//UNIX 域套接字只能用于在同一个计算机的进程间通信。
//UNIX 域套接字仅仅进行数据复制，不会执行在网络协议栈中需要处理的添加、删除报文头、计算校验和、计算报文顺序等复杂操作，因而在本地的进程间通信中，更加推荐使用 UNIX 域套接字。
//当套接字用于本地通信时，可以用结构体 struct sockaddr_un 描述一个本地地址。

//UNIX 域流式套接字
//（1）创建 UNIX 域流式套接字。
//（2）将套接字文件描述符与本地信息结构体绑定。
//（3）设置监听模式。
//（4）接收客户端的连接请求。
//（5）发送、 接收数据。
void unix_server_test() {
    sockaddr_un addr, client;
    socklen_t addr_len = sizeof(addr);

    //第一步：创建套接字
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    //第二步：填充服务器本地信息结构体
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "unix");
    //第三步：将套接字与服务器网络信息结构体绑定
    bind(fd, (struct sockaddr *) &addr, addr_len);
    //第四步：将套接字设置为被动监听模式
    listen(fd, 5);

    //第五步：阻塞等待客户端的连接请求
    int accept_fd = accept(fd, (struct sockaddr *) &client, &addr_len);

    int bytes;
    char buf[1024] = {0};

    while (1) {
        if ((bytes = recv(accept_fd, buf, sizeof(buf), 0)) < 0) {
            perror("recv error");
        } else if (bytes == 0) {
            perror("no data");
            exit(1);
        } else {
            if (strncmp(buf, "exit", 4) == 0) {
                printf("client exit\n");
                break;
            } else {
                buf[bytes] = '\0';
                printf("client: %s\n", buf);
                strcat(buf, "-server");

                if (send(accept_fd, buf, strlen(buf), 0) < 0) {
                    perror("send error");
                }
            }
        }
    }

    close(accept_fd);
    close(fd);
}

//UNIX 域流式套接字实现本地通信客户端流程如下所示。
//（1）创建 UNIX域流式套接字。
//（2）指定服务器端地址（套接字文字）。
//（3）建立连接。
//（4）发送、 接收数据
void unix_client_test() {
    sockaddr_un addr;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "unix");

    connect(fd, (struct sockaddr *) &addr, addr_len);

    char buf[1024] = {0};
    int bytes;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        if (send(fd, buf, strlen(buf), 0) < 0) {
            perror("send error");
        } else {
            if (strncmp(buf, "exit", 4) == 0) {
                printf("client exit\n");
                break;
            }

            if ((bytes = recv(fd, buf, sizeof(buf), 0)) < 0) {
                perror("recv error");
            }

            buf[bytes] = '\0';
            printf("%s\n", buf);
        }
    }

    close(fd);
}

//UNIX 域数据报套接字
//（1）创建 UNIX 域数据报套接字。
//（2）填充服务器本地信息结构体。
//（3）将套接字与服务器本地信息结构体绑定。
//（4）进行通信（recvfrom()函数/sendto()函数）。

void unix_udp_server_test() {
    sockaddr_un addr, client;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "unix");

    bind(fd, (struct sockaddr *) &addr, addr_len);

    int bytes;
    char buf[1024] = {0};

    while (1) {
        if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &client, &addr_len)) < 0) {
            perror("recvfrom error");
        } else {
            printf("client: %s\n", client.sun_path);

            if (strncmp(buf, "exit", 4) == 0) {
                printf("client exit\n");
                break;
            }

            buf[bytes] = '\0';
            printf("client: %s\n", buf);
            strcat(buf, "-server");
            if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &client, addr_len) < 0) {
                perror("sendto error");
            }
        }
    }
    close(fd);
}

//（1）创建 UNIX 域数据报套接字。
//（2）填充服务器本地信息结构体。
//（3）填充客户端本地信息结构体。
//（4）将套接字与客户端本地信息结构体绑定。
//（5）进行通信（sendto()/recvfrom()）。

//UNIX 域数据报套接字与网络数据报套接字（UDP）不同的是，前者的客户端一定要绑定自己的信息结构体。
//数据报套接字在用于网络通信时，服务器可以为客户端自动分配地址和端口。
//而在本地通信时，服务器不会为客户端分配套接字文件（本地信息结构体）。 如果客户端不绑定自己的信息结构体，服务器则无法知道客户端是谁。
void unix_udp_client_test() {
    sockaddr_un addr, client;
    socklen_t addr_len = sizeof(addr);

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "unix");

    //客户端需要绑定自己的信息,否则服务器无法给客户端发送数据
    client.sun_family = AF_UNIX;
    strcpy(client.sun_path, "unix_client");
    bind(fd, (struct sockaddr *) &client, addr_len);

    char buf[1024] = {0};
    int bytes;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';
        if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &addr, addr_len) < 0) {
            perror("sendto error");
        }

        if (strncmp(buf, "exit", 4) == 0) {
            printf("client exit\n");
            break;
        } else {
            if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (struct sockaddr *) &addr, &addr_len)) < 0) {
                perror("recvfrom error");
            }
            buf[bytes] = '\0';
            printf("server: %s\n", buf);
        }
    }
}

//原始套接字
//原始套接字（SOCK_RAW）不同于流式套接字、数据报套接字。原始套接字是基于 IP 数据包的编程，流式套接字只能收发 TCP 的数据，数据报套接字只能收发 UDP 的数据。
//协议栈的原始套接字从实现上可以分为链路层原始套接字和网络层原始套接字两大类。
//
//（1）进程使用原始套接字可以读写 ICMP、IGMP 等网络报文。
//（2）进程使用原始套接字就可以读写那些内核不处理的 IPv4 数据报。
//（3）进程可以使用 IP_HDRINCL 套接字选项自行构造 IP 头部。

//链路层原始套接字的创建
//第一个参数指定协议族类型为 AF_PACKET，第二个参数可以设置为 SOCK_RAW 或 SOCK_DGRAM

//参数 type 设置为 SOCK_RAW 时，套接字接收和发送的数据都是从 MAC 首部开始的。在发送时需要由调用者从 MAC 首部开启构造和封装报文数据。
//socket(AF_PACKET，SOCK_RAW，htons(protocol));

//参数 type 设置为 SOCK_DGRAM 时，套接字接收的数据报文会将 MAC 头部去掉。同时在发送时也不需要再手动构造 MAC 头部。只需要从 IP 头部开始构造数据即可
//socket(AF_PACKET，SOCK_DGRAM，htons(protocol));

//链路层原始套接字的创建。
void raw_socket_test() {
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket error");
        return;
    }
    printf("fd: %d\n", fd);
    close(fd);
}

//网络层原始套接字的创建
//创建面向连接的 TCP 和创建面向无连接的 UDP 套接字，在接收和发送时只能操作数据部分，而不能操作 IP、TCP 和 UDP 头部。
//如果想要操作 IP头部或传输层协议头部，则需要调用 socket()函数创建网络层原始套接字。
//第一个参数指定协议族的类型为 AF_INET，第二个参数为 SOCK_RAW，第三个参数 protocol 为协议类型。

//ICMP、Ethernet 封包解析
//例如，使用 TCP 发送应用数据时，则应用数据经过传输层时会在数据包之前再添加一个头信息，这个头信息即为 TCP 头部。
//依次类推，数据包在应用层发送到底层（物理层）的过程中，不断地对其数据包进行封装，经过的每一层都会对数据“安装”一个头部，用来实现该层的功能。
//反之，当接收数据包时，则会在经过的每一层中 对数据包进行“拆解” ，将对应该层协议的头部解析掉。 这样就完成了一个数据包的基本传递。

//MAC 数据包接收
void mac_packet_test() {
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket error");
        return;
    }
    printf("fd: %d\n", fd);

    int ret;
    unsigned short mac_type;

    while (1) {
        unsigned char buf[1600] = {0};
        char src_mac[18] = {0};
        char dst_mac[18] = {0};

        //buf 将存放完整的帧数据 （例如，MAC 头部+IP 头+TCP/UDP 头+应用数据）
        ret = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);

        sprintf(src_mac, "%x:%x:%x:%x:%x:%x", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
        sprintf(dst_mac, "%x:%x:%x:%x:%x:%x", buf[0 + 6], buf[1 + 6], buf[2 + 6], buf[3 + 6], buf[4 + 6], buf[5 + 6]);

        //从封包中获取网络层的数据类型，注意字节序的转换
        mac_type = ntohs(*(unsigned short *) (buf + 12));

        printf("源mac %s => 目的mac %s\n", src_mac, dst_mac);
        printf("mac_type = %x\n", mac_type);

        if (mac_type == 0x800) {
            printf("IP数据包\n");

            //定义指针变量保存 IP 数据包首地址
            unsigned char *ip_buf = buf + 14;

            //获取 IP 数据包首部长度
            //首部长度占 4 位，是数据包的前八位中的后四位，且单位是 4 字节
            int ip_head_len = ((*ip_buf) & 0x0f) * 4;
            printf("IP数据包首部长度 %d 字节\n", ip_head_len);
            //获取 IP 封包的总长度，表示总长度的位置从 IP 数据包的第 16 位开始
            printf("IP数据包总长度 %d 字节\n", ntohs(*(unsigned short *) (ip_buf + 2)));

            //分析 IP 协议包中传输层协议(IP 报文的上层协议)
            unsigned char ip_type = *(unsigned short *) (ip_buf + 9);
            if (ip_type == 1) {
                printf("ICMP\n");
            } else if (ip_type == 2) {
                printf("IGMP\n");
            } else if (ip_type == 6) {
                printf("TCP\n");
            } else if (ip_type == 17) {
                printf("UDP\n");
            }
        } else if (mac_type == 0x0806) {
            printf("ARP数据包\n");
        } else if (mac_type == 0x8035) {
            printf("RARP数据包\n");
        }
    }

    close(fd);
}

//MAC 数据包发送
//sendto(sockfd, buf, len, 0, dest_addr, addrlen);
//int ioctl(int d, int request, ...);
//ioctl()函数是一个功能非常强大的函数，被用来实现对文件描述符的控制。

//ARP 实现 MAC 地址扫描
//ARP 主要的功能是可以查询指定 IP 所对应的 MAC，具体的工作原理如下。
//（1）每个主机都会在自己的 ARP 缓存区中建立一个 ARP 列表，以表示 IP 地址和 MAC 地址 之间的对应关系。
//（2）当有主机新加入网络时（也有可能是网络接口重启），会发送免费 ARP 报文把自己的 IP 地 址和 MAC 地址的映射关系广播给其他主机。
//（3）网络上的主机接收免费 ARP 报文时，会更新自己的 ARP 缓存区，将新的映射关系更新到 自己的 ARP 表中。
//（4）当某个主机需要发送报文时，首先检查 ARP 列表中是否有对应 IP 地址的目的主机的 MAC 地址。如果有，则直接发送数据；如果没有，则向本网段中的所有主机发送 ARP 数据报（广播）， 包括源主机 IP 地址、源主机 MAC 地址、目的主机 IP 地址等。
//（5）当本网络的所有主机收到该 ARP 数据报时，首先检测数据报中的 IP 地址是否为自己的 IP 地址，如果不是，则忽略该数据报；如果是，则首先从数据报中取出源主机的 IP 和 MAC 地址更新 到 ARP 列表中，如果已经存在，则覆盖；然后将自己的 MAC 地址写入 ARP 响应包中，通知源主 机自己就是它想要寻找的 MAC 地址。
//（6）源主机收到 ARP 响应包后，将目的主机的 IP 和 MAC 地址写入 ARP 列表，之后则可以发 送数据。如果源主机一直没有收到 ARP 响应数据包，表示 ARP 查询失败。

//通过 ARP 协议获取目的主机的 MAC 地址。
void arp_get_mac() {
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket error");
        return;
    }
    //
    //MAC头 ARP头
    unsigned char msg[1600] = {
        //目的 MAC,广播请求所使用的 MAC 地址
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        //源MAC
        0x00, 0x0c, 0x29, 0x07, 0x2f, 0xd3,
        //帧类型 ARP
        0x08, 0x06,
        //硬件类型（以太网地址为 1）
        0x00, 0x01,
        //协议类型
        0x08, 0x00,
        //硬件地址长度和协议地址长度（MAC 和 IP）
        0x06, 0x04,
        //选项，ARP 请求
        0x00, 0x01,
        //源 MAC
        0x00, 0x0c, 0x29, 0x07, 0x2f, 0xd3,
        //源 IP 地址
        192, 168, 1, 222,
        //目的 MAC
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        //目的 IP 地址
        192, 168, 1, 4
    };

    //将 ARP 请求报文通过 eth0 发送出去
    //使用 ioctl()函数获取本机网络接口
    struct ifreq req;
    strncpy(req.ifr_name, "ens160", IFNAMSIZ);

    //SIOCGIFINDEX 获取网络接口
    if (ioctl(fd, SIOCGIFINDEX, &req) == -1) {
        perror("ioctl error");
        return;
    }

    //设置本机网络接口
    struct sockaddr_ll addr;
    addr.sll_ifindex = req.ifr_ifindex;

    sendto(fd, msg, 42, 0, (struct sockaddr *) &addr, sizeof(addr));

    while (1) {
        unsigned char buf[1600] = {0};

        recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);

        unsigned short mac_type = ntohs(*(unsigned short *) (buf + 12));
        //必须是 ARP 数据包才分析
        if (mac_type == 0x0806) {
            //请求为 ARP 应答
            unsigned short arpopt = htons(*(unsigned short *) (buf + 20));
            if (arpopt == 0x02) {
                char mac[18] = {0};
                sprintf(mac, "%x:%x:%x:%x:%x:%x", buf[0 + 6], buf[1 + 6], buf[2 + 6], buf[3 + 6], buf[4 + 6],
                        buf[5 + 6]);
                printf("192.168.1.4 ==> %s\n", mac);
                break;
            }
        }
    }
    close(fd);
}

struct arphdr
{
    unsigned short int ar_hrd;		/* Format of hardware address.  */
    unsigned short int ar_pro;		/* Format of protocol address.  */
    unsigned char ar_hln;		/* Length of hardware address.  */
    unsigned char ar_pln;		/* Length of protocol address.  */
    unsigned short int ar_op;		/* ARP opcode (command).  */
    unsigned char __ar_sha[ETH_ALEN];	/* Sender hardware address.  */
    unsigned char __ar_sip[4];		/* Sender IP address.  */
    unsigned char __ar_tha[ETH_ALEN];	/* Target hardware address.  */
    unsigned char __ar_tip[4];		/* Target IP address.  */
};

void arp_get_mac2() {
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket error");
        return;
    }
    unsigned char msg[1600] = {0};
    unsigned char src_mac[6] = {0x00, 0x0c, 0x29, 0x07, 0x2f, 0xd3};

    struct ether_header *eth_head = (struct ether_header *) msg;
    //目的 MAC
    memset(eth_head->ether_dhost, 0xff, 6);
    //源 MAC
    memcpy(eth_head->ether_shost, src_mac, 6);
    //协议类型
    eth_head->ether_type = htons(0x0806);

    //组 ARP 头
    struct arphdr *arp_head = (struct arphdr*)(msg + 14);
    //硬件类型，以太网为 1
    arp_head->ar_hrd = htons(1);
    //协议类型
    arp_head->ar_pro = htons(0x0800);
    //硬件地址长度
    arp_head->ar_hln = 6;
    //协议地址长度
    arp_head->ar_pln = 4;
    //ARP 请求
    arp_head->ar_op = htons(1);
    //源 MAC
    memcpy(arp_head->__ar_sha, src_mac, 6);
    //源 IP
    *(unsigned int*)(arp_head->__ar_sip) = inet_addr("192.168.1.222");
    //目的 MAC
    memset(arp_head->__ar_tha, 0, 6);
    //目的 IP
    *(unsigned int *)(arp_head->__ar_tip) = inet_addr("192.168.1.4");

    struct ifreq req;
    strncpy(req.ifr_name, "ens160", IFNAMSIZ);

    //SIOCGIFINDEX 获取网络接口
    if (ioctl(fd, SIOCGIFINDEX, &req) == -1) {
        perror("ioctl error");
        return;
    }

    struct sockaddr_ll addr;
    addr.sll_ifindex = req.ifr_ifindex;

    sendto(fd, msg, 42, 0, (struct sockaddr *) &addr, sizeof(addr));

    //接收数据
    while (1) {
        unsigned char buf[1600] = {0};

        recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);

        unsigned short mac_type = ntohs(*(unsigned short *) (buf + 12));
        //必须是 ARP 数据包才分析
        if (mac_type == 0x0806) {
            //请求为 ARP 应答
            unsigned short arpopt = htons(*(unsigned short *) (buf + 20));
            if (arpopt == 0x02) {
                char mac[18] = {0};
                sprintf(mac, "%x:%x:%x:%x:%x:%x", buf[0 + 6], buf[1 + 6], buf[2 + 6], buf[3 + 6], buf[4 + 6],
                        buf[5 + 6]);
                printf("192.168.1.4 ===> %s\n", mac);
                break;
            }
        }
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    //timeout_test();
    //select_timeout_test();
    //signal_timeout_test();
    //if (strcmp(argv[1], "server") == 0) {
    //    broadcast_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    broadcast_client_test();
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    multicast_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    multicast_client_test();
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    unix_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    unix_client_test();
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    unix_udp_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    unix_udp_client_test();
    //}
    //mac_packet_test();
    //arp_get_mac();
    arp_get_mac2();
    return 0;
}
