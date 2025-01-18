#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

//TCP 编程
//套接字编程的基本函数有 socket()、bind()、listen()、accept()、send()、sendto()、 recv()及 recvfrom()等。

//socket()函数用于创建一个套接字，同时指定协议和类型。套接字是一个允许通信的“设备” ，两个应用程序通过它完成数据的传递。
//bind()函数将保存在相应地址结构中的地址与套接字进行绑定。 它主要 用于服务器端。 客户端创建的套接字可以不绑定地址。
//listen()函数表示监听，在服务器端程序成功创建套接字并与地址进行绑 定之后，通过调用 listen()函数将套接字设置为监听模式（被动模式），准备 接收客户端的连接请求。
//accept()函数表示接收，服务器通过调用 accept()函数等待并接收客户端的连接请求。当建立好TCP 连接后，该操作将返回一个新的已连接套接字。
//connect()函数表示连接，客户端通过该函数向服务器端的监听套接字发送连接请求。
//send()和 recv()两个函数在 TCP 通信过程中用于发送和接收数据，也可以用在 UDP 中。 sendto()和 recvfrom()两个函数一般用在 UDP 通信中，用于发送和接收数据。

//创建套接字
//int socket(int domain, int type, int protocol);
//TCP 服务器接口
//int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
//监听
//int listen(int sockfd, int backlog);
//连接请求
//int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
//TCP 客户端接口
//int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
//发送数据
//ssize_t send(int sockfd, const void *buf, size_t len, int flags);
//接收数据
//ssize_t recv(int sockfd, void *buf, size_t len, int flags);
void tcp_server_test() {
    sockaddr_in addr;
    //创建套接字
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    //填充网络信息结构体
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9999);
    //将套接字与服务器网络信息结构体绑定
    bind(fd, (sockaddr *) &addr, sizeof(addr));
    //将套接字设置为被动监听模式
    listen(fd, 5);

    //阻塞等待客户端的连接请求
    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(fd, (sockaddr *) &client, &client_len);
    printf("client ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

    ssize_t bytes = 0;
    char buf[1024] = {0};

    while (1) {
        bytes = recv(client_fd, buf, sizeof(buf), 0);
        if (bytes < 0) {
            perror("recv error");
        } else if (bytes == 0) {
            perror("no data");
        } else {
            if (strncmp(buf, "exit", 4) == 0) {
                printf("client exit\n");
                goto ERR;
            }

            printf("%d\n", bytes);
            buf[bytes] = '\0';
            printf("client %s\n", buf);
            strcat(buf, "-server");

            send(client_fd, buf, strlen(buf), 0);
        }
    }
ERR:
    close(client_fd);
    close(fd);
}

void tcp_client_test() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    //
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9999);
    //绑定
    bind(fd, (sockaddr *) &addr, sizeof(addr));
    //发送客户端连接请求
    connect(fd, (sockaddr *) &addr, sizeof(addr));

    ssize_t bytes = 0;
    char buf[1024] = {0};

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        if (send(fd, buf, strlen(buf), 0) < 0) {
            perror("send error");
        } else {
            if (strncmp(buf, "exit", 4) == 0) {
                goto ERR;
            }

            if ((bytes = recv(fd, buf, sizeof(buf), 0)) < 0) {
                perror("recv error");
            }

            buf[bytes] = '\0';
            printf("server %s\n", buf);
        }
    }
ERR:
    close(fd);
}

//UDP 编程
//UDP 不同于 TCP 的是面向无连接，使用 UDP 通信时服务器端和客户端无须建立连接，只需知道对方套接字的地址信息，就可以发送数据。

//发送和接收数据
//ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
//ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

void udp_server_test() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9999);

    bind(fd, (sockaddr *) &addr, sizeof(addr));

    ssize_t bytes = 0;
    sockaddr_in client;
    socklen_t client_len = sizeof(client);
    char buf[1024] = {0};

    while (1) {
        if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (sockaddr *) &client, &client_len)) < 0) {
            perror("recvfrom error");
        } else {
            printf("ip %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

            if (strncmp(buf, "exit", 4) == 0) {
                printf("server exit\n");
                break;
            }

            buf[bytes] = '\0';
            printf("client %s\n", buf);

            strcat(buf, "-server");

            sendto(fd, buf, strlen(buf), 0, (sockaddr *) &client, client_len);
        }
    }
}

void udp_client_test() {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(9999);

    char buf[1024] = {0};
    ssize_t bytes = 0;

    while (1) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';

        sendto(fd, buf, strlen(buf), 0, (sockaddr *) &addr, addr_len);

        if (strncmp(buf, "exit", 4) == 0) {
            printf("client exit\n");
            break;
        }

        if ((bytes = recvfrom(fd, buf, sizeof(buf), 0, (sockaddr *) &addr, &addr_len)) < 0) {
            perror("recvfrom error");
        }

        buf[bytes] = '\0';
        printf("server %s\n", buf);
    }
}

//TCP 三次握手和四次挥手
//TCP 是面向连接且具有可靠传输的协议。
//当建立一个TCP 连接时，需要客户端和服务器端总共发送3个包以确认连接的建立，这个过程被称为“ 三次握手” 。
//而当断开 TCP 连接时，需要客户端和服务器端总共发送4个包以确认连接的断开，这个过程被称为“ 四次挥手” 。
//三次握手的过程如下。
//（1）服务器通常通过调用 socket()、 bind()和 listen()这 3 个函数准备好接收外来的连接，称之为 被动打开。
//（2）客户端通过调用 connect()函数执行主动打开，这将导致客户端发送一个同步序列编号 （Synchronize Sequence Numbers，SYN）分节，以告诉服务器客户端在（待建立的）连接中发送的数 据的序列号。 通常，SYN 分节不携带数据。
//（3）服务器通过发送 ACK 报文确认客户端的 SYN，同时自己也发送一个 SYN 分节，这个 SYN 分节含有服务器将在同一连接中发送数据的初始序列号。
//（4）客户端必须确认服务器的 SYN。

//四次挥手的过程如下。
//（1）某个应用进程首先调用 close()函数，称该端执行主动关闭。 该端的 TCP 于是发送一个 FIN 分节，表示数据发送完毕。
//（2）接收这个 FIN 的对端执行被动关闭。它的接收也作为一个文件结束符传递给接收端应用进 程。FIN 的接收意味着接收端应用进程在相应连接上再无额外数据可接收。
//（3）一段时间后，接收这个文件结束符的应用进程将调用 close()函数关闭它的套接字，这导致 它的 TCP 也发送一个 FIN。
//（4）接收这个最终的 FIN 的原发送端 TCP（执行主动关闭的那一端）确认这个 FIN。

//数据包封装与解析
//TCP/IP 体系结构。该结构定义了分层的思想，一共分为四层，分别为应用层、传输层、 网络层、 网络接口和物理层。

//TCP、UDP、IP 封包格式
//1．TCP 报文格式
//（1）源端口号：TCP 发送端的端口号。
//（2）目的端口号：TCP 接收端的端口号。
//（3）序列号：该报文的序列号，标识从 TCP 发送端向 TCP 接收端发送的数据字节流。避免出 现接收重复包，以及可以对乱序数据包进行重排序。
//（4）确认序号：标识了报文发送端期望接收的字节序列。注意它是一个准备接收的包的序 列号。
//（5）头部长度：该字段用来表示 TCP 报文头部的长度，头部长度单位是 32 位（4 字节）。由于 这个字段只占 4 位，因此头部总长度最大可达到 60 字节（15 个字长）。该字段使得 TCP 接收端可 以确定变长的选项字段的长度，以及数据域的起始点。
//（6）保留位：该字段包含 4 位（未使用，必须置为 0）。
//（7）控制位：该字段由 8 位组成，用于进一步指定报文的含义。
//    ① CW R：拥塞窗口减小标记。
//    ② EC E：显式的拥塞通知回显标记。
//    ③ URG ：如果设置了该位，那么紧急指针字段包含的信息就是有效的。
//    ④ ACK ：如果设置了该位，那么确认序号字段包含的信息就是有效的（该字段用于确认由对 端发送过来的上一个数据）。
//    ⑤ PSH ：将所有收到的数据发给接收的进程。
//    ⑥ RST ：重置连接。该字段用来处理多种错误情况。
//    ⑦ SYN ：同步序列号。在建立连接时，双方需要交换该位的报文。这样使得 TCP 连接的两端 可以指定初始序列号，稍后用于双向传输数据。
//    ⑧ FIN ：发送端提示已经完成了发送任务。
//（8）窗口大小：该字段用在接收端发送 ACK 确认时提示自己可接收数据的空间大小。
//（9）TCP 校验和：奇偶校验，此校验和是针对整个的 TCP 报文段，包括 TCP 头部和 TCP 数据。 由发送端计算，接收端进行验证。
//（10）紧急指针：如果设定了 URG 位，那么就表示从发送端到接收端传输的数据为紧急数据。
//（11）选项：这是一个变长的字段，包括了控制 TCP 连接操作的选项。
//（12）数据：这个字段包含了该报文段中传输的用户数据。如果报文段没有包含任何数据的话， 这个字段的长度就为 0。

//2．UDP 数据报封包格式
//（1）源端口号：UDP 发送端的端口号。
//（2）目的端口号：UDP 接收端的端口号。
//（3）UDP 长度：UDP 头部和 UDP 数据的字节长度。
//（4）UDP 校验和：UDP 校验和是一个端到端的检验和。它由发送端计算，然后由接收端验证。

//3．IP 数据报的格式
//（1）版本。目前协议的版本号为 4，因此 IP 有时也称为 IPv4。
//（2）头部长度。头部长度指 IP 头部的大小，包括任何选项，其单位为 32 位（4 字节）。
//由于它是一个 4 位字段，因此头部最长为 60 字节（4 位头部长度字段所能表示的最大值为 1111，转化为十进制为 15，15×32/8 = 60）。
//（3）服务类型。服务类型字段包括一个 3 位的优先权子字段，4 位的 TOS 子字段和 1 位未用位（但必须置 0）。
//TOS 字段为 4 位，第 1 位表示最小时延，第 2 位表示最大吞吐量，第 3 位表示最高可靠性，第 4 位表示最小费用。如果修改这个字段，只能选择 4 位中的 1 位进行操作。如果所有 4位均为 0，那么就意味着是一般服务。
//（4）总长度。总长度指整个 IP 数据报的长度，以字节为单位。利用头部长度字段和总长度字段，就可以知道 IP 数据报中数据内容的起始位置和长度。
//由于该字段长 16 位，所以 IP 数据报最长可达65535 字节。总长度字段是 IP 头部中必要的内容，因为一些数据链路（如以太网）需要填充一些数据以达到最小长度。
//尽管以太网的最小帧长为 46 字节，但是 IP 数据可能会更短。如果没有总长度字段，那么 IP 层就无法知道 46 字节中 IP 数据报的内容长度。
//（5）标识。标识是指主机发送的每一份数据报。通常每发送一份报文，其值就会加 1。
//（6）生存时间（TTL）。生存时间字段设置了数据报可以经过的最多路由器数。它指定了数据报的生存时间。
//TTL 的初始值由源主机设置（通常为 32 或 64），一旦经过一个处理它的路由器，它的值就减 1。当该字段的值为 0 时，数据报就会被丢弃，并发送 ICMP 报文通知源主机。
//（7）任选项。任选项是数据报中的一个可变长的可选信息。目前，这些任选项包括安全和处理限制、记录路径、时间戳、宽松的源站选路、严格的源站选路（与宽松的源站选路类似，但是要求只能经过指定的这些地址，不能经过其他的地址）。这些选项很少被使用，并非所有的主机和路由器
//都支持这些选项。选项字段一直都是以 32 位作为界限，在必要的时候插入值为 0 的填充字节。这样就保证 IP 头部始终是 32 位的整数倍（这是头部长度字段所要求的）。

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "server") == 0) {
    //    tcp_server_test();
    //} else if (strcmp(argv[1], "client") == 0) {
    //    tcp_client_test();
    //}
    if (strcmp(argv[1], "server") == 0) {
        udp_server_test();
    } else if (strcmp(argv[1], "client") == 0) {
        udp_client_test();
    }
    return 0;
}
