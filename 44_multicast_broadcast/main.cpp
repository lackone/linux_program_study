#include <cstring>
#include <iostream>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
using namespace std;

//多播与广播
//多播方式的数据传输是基于UDP完成的。采用多播方式时，可以同时向多个主机传递数据。
//1、多播服务器端针对特定多播组，只发送1次数据。
//2、即使只发送1次数据，但该组内的所有客户端都会接收数据
//3、多播组数可以在IP地址范围内任意增加
//4、加入特定组即可接收发往该多播组的数据
//多播组是D类IP地址（224.0.0.0 - 239.255.255.255)

//路由 routing 和 TTL (time to live 生存时间） 以及加组方法
// TTL 是决定数据包传递距离 的主要因素，每经过1个路由器就减1，TTL变为0时，数据包无法再被传递。
// int time_live = 64;
// setsockopt(send_sock, IPPROTO_IP, IP_MULTICAST_TTL, (void*)&time_live, sizeof(time_live));
// 加入多播组
// struct ip_mreq join;
// join.imr_multiaddr.s_addr = "多播组地址信息";
// join.imr_interface.s_addr = "加入多播组的主机地址信息";
// setsockopt(send_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void*)&join, sizeof(join));

#if defined(__linux__) || defined(__unix__)

void multicast_server(int port) {
    sockaddr_in addr;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); //多播端口
    addr.sin_addr.s_addr = inet_addr("224.1.1.1"); //多播组IP

    int ttl = 64;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    char buf[256] = {0};
    while (1) {
        fgets(buf, sizeof(buf), stdin);

        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
        }

        sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &addr, sizeof(addr));
    }

    close(sock);
}

void multicast_client(int port) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct ip_mreq req;
    req.imr_multiaddr.s_addr = inet_addr("224.1.1.1");
    req.imr_interface.s_addr = htonl(INADDR_ANY);

    //这里需要bind，不然无法收到消息
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));

    //加入多播组
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &req, sizeof(req));

    int len;
    char buf[256] = {0};
    while (1) {
        len = recvfrom(sock, buf, sizeof(buf), 0, NULL, 0);
        if (len < 0) {
            break;
        }
        buf[len] = '\0';
        printf("%s\n", buf);
    }

    close(sock);
}

//广播
//多播即使在跨越不同网络的情况下，只要加入多播组就能接收数据，相反，广播只能向同一网络中的主机传输数据。
//广播也是基于UDP完成的。
//1、直接广播
//2、本地广播
//直接广播的IP地址除网络地址外，主机地址全部为1，比如向192.168.1中所有主机传输数据，可以向 192.168.1.255传输。
//本地广播使用的IP地址限定为 255.255.255.255，比如192。168.1主机向255.255.255.255传输数据，数据将传递到192.168.1所有主机

//默认生成的套接字会阻止广播
//int bcast = 1;
//setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));

void broadcast_server(int port) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    int bcast = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast));

    char buf[256] = {0};

    while (1) {
        fgets(buf, sizeof(buf), stdin);

        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
        }

        sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &addr, sizeof(addr));
    }
    close(sock);
}

void broadcast_client(int port) {
    sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(sock, (struct sockaddr *) &addr, sizeof(addr));

    int len;
    char buf[256] = {0};

    while (1) {
        len = recvfrom(sock, buf, sizeof(buf), 0, NULL, 0);
        if (len < 0) {
            break;
        }
        buf[len] = '\0';
        printf("%s\n", buf);
    }
    close(sock);
}

#else

void windows_multicast_server(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN addr;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); //多播端口
    addr.sin_addr.s_addr = inet_addr("224.1.1.1"); //多播组IP

    int ttl = 64;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &ttl, sizeof(ttl));

    char buf[256] = {0};
    while (1) {
        fgets(buf, sizeof(buf), stdin);

        if (strncmp(buf, "q", 1) == 0 ||
            strncmp(buf, "Q", 1) == 0) {
            break;
        }

        sendto(sock, buf, strlen(buf), 0, (struct sockaddr *) &addr, sizeof(addr));
    }

    closesocket(sock);
    WSACleanup();
}

void windows_multicast_client(int port) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    SOCKADDR_IN addr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    struct ip_mreq req;
    req.imr_multiaddr.s_addr = inet_addr("224.1.1.1");
    req.imr_interface.s_addr = htonl(INADDR_ANY);

    //这里需要bind，不然无法收到消息
    bind(sock, (struct sockaddr *) &addr, sizeof(addr));

    //加入多播组
    setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &req, sizeof(req));

    int len;
    char buf[256] = {0};
    while (1) {
        len = recvfrom(sock, buf, sizeof(buf), 0, NULL, 0);
        if (len < 0) {
            break;
        }
        buf[len] = '\0';
        printf("%s\n", buf);
    }

    closesocket(sock);
    WSACleanup();
}

#endif

int main(int argc, char *argv[]) {
    //if (strcmp(argv[1], "server") == 0) {
    //    multicast_server(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    multicast_client(atoi(argv[2]));
    //}
    //if (strcmp(argv[1], "server") == 0) {
    //    broadcast_server(atoi(argv[2]));
    //} else if (strcmp(argv[1], "client") == 0) {
    //    broadcast_client(atoi(argv[2]));
    //}
    if (strcmp(argv[1], "server") == 0) {
        windows_multicast_server(atoi(argv[2]));
    } else if (strcmp(argv[1], "client") == 0) {
        windows_multicast_client(atoi(argv[2]));
    }
    return 0;
}
