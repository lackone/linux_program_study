#include <iostream>
#include <cstring>
#include <io.h>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

//分配给套接字的IP地址与端口号
//网络地址
//IPV4   4字节地址族
//IPV6   16字节地址族
//IPV4标准的4字节IP地址分为网络地址和主机地址，且分为A,B,C,D,E等类型。
//网络地址是为区分网络而设置的一部分IP地址
//A类地址的首字节范围 0 - 127
//B类地址的首字节范围 128 - 191
//C类地址的首字节范围 192 - 223

//端口号就是在同一操作系统内为区分不同套接字而设置的。因此无法将1个端口号分配给不同套接字。
//端口号由16位组成，范围 0~65535

//struct sockaddr_in {
//    short sin_family; //地址族
//    u_short sin_port;  //端口号
//    struct in_addr sin_addr; //32位IP地址
//    char sin_zero[8]; //不使用
//};

//struct in_addr {
//    In_addr_t s_addr; //32位IPV4地址
//};

//sin_family，地址族， AF_INET  AF_INET6  AF_LOCAL
//sin_port  16位端口号
//sin_addr  32位IP地址
//sin_zero  无特殊含义

//网络字节序与地址变换
//字节序与网络字节序
//大端序，高位字节存放到低位地址
//小端序，高位字节存放到高位地址
//整数 0x12345678，0x12是高位字节，0x78是低位字节，大端序中，高位存放在低位，0x12 0x34 0x56 0x78
//小端序，高位存放在高位，0x78 0x56 0x34 0x12
//intel系列CPU以小端序方式保存数据
//通过网络传输数据时约定统一方式，这种约定称为网络字节序，统一为大端序。

//字节序转换
//htons()
//ntohs()
//htonl()
//ntohl()
void order_test() {
    unsigned short host_port = 0x1234;
    unsigned short net_port;
    unsigned long host_addr = 0x12345678;
    unsigned long net_addr;

    net_port = htons(host_port);
    net_addr = htonl(host_addr);

    printf("host_port = %x\n", host_port);
    printf("net_port = %x\n", net_port);
    printf("host_addr = %x\n", host_addr);
    printf("net_addr = %x\n", net_addr);
}

//网络地址的初始化与分配
//将字符串信息转换为网络字节序的整数型
//in_addr_t inet_addr(const char* string);

//int inet_aton(const char* string, struct in_addr *addr);

//char* inet_ntoa(struct in_addr adr);

void inet_addr_test() {
    const char *addr1 = "1.2.3.4";
    const char *addr2 = "1.2.3.256";

    long addr;

    if ((addr = inet_addr(addr1)) == INADDR_NONE) {
        printf("error\n");
    } else {
        printf("%lx\n", addr);
    }

    if ((addr = inet_addr(addr2)) == INADDR_NONE) {
        printf("error\n");
    } else {
        printf("%lx\n", addr);
    }

    const char *addr3 = "127.232.124.79";
    sockaddr_in addr_in;

    if (!inet_pton(AF_INET, addr3, &addr_in.sin_addr)) {
        printf("error\n");
    } else {
        printf("%lx\n", addr_in.sin_addr.s_addr);
    }

    sockaddr_in addr_in2;
    addr_in2.sin_addr.s_addr = htonl(0x1020304);
    printf("%x\n", htonl(0x1020304));
    char *str = inet_ntoa(addr_in2.sin_addr);
    printf("%s\n", str);
}

//INADDR_ANY
//自动获取运行服务器端的计算机IP地址

//WSAStringToAddress 和 WSAAddressToString
void wsa_string_address_test() {
    char *addr = "203.211.218.102:9190";

    SOCKADDR_IN addr_in;
    int size = sizeof(addr_in);
    char buf[50];

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    WSAStringToAddress(addr, AF_INET, NULL, (SOCKADDR *) &addr_in, &size);

    printf("%lx\n", addr_in.sin_addr.s_addr);

    size = sizeof(buf);
    WSAAddressToString((SOCKADDR *) &addr_in, sizeof(addr_in), NULL, buf, (LPDWORD) &size);

    printf("%s\n", buf);

    WSACleanup();
}

int main() {
    //order_test();
    inet_addr_test();
    wsa_string_address_test();
    return 0;
}
