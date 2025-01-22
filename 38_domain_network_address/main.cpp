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

//域名系统
//DNS是对IP地址和域名进行相互转换的系统，其核心是DNS服务器

//IP地址和域名之间的转换

//利用域名获取IP地址
//struct hostent * gethostbyname(const char* hostname);

#if defined(__linux__) || defined(__unix__)

void gethostbyname_test(const char *hostname) {
    struct hostent *he = gethostbyname(hostname);

    printf("name %s\n", he->h_name);

    for (int i = 0; he->h_aliases[i]; i++) {
        printf("aliases %s\n", he->h_aliases[i]);
    }

    printf("type = %s\n", he->h_addrtype == AF_INET ? "AF_INET" : "AF_INET6");

    for (int i = 0; he->h_addr_list[i]; i++) {
        printf("ip addr %s\n", inet_ntoa(*(struct in_addr *) he->h_addr_list[i]));
    }
}

//利用IP地址获取域名
//struct hostent *gethostbyaddr(const char* addr, socklen_t len, int family);

void gethostbyaddr_test(const char *ip) {
    sockaddr_in addr;

    printf("%s\n", ip);
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(ip);

    struct hostent *he = gethostbyaddr((char*)&addr.sin_addr, 4, AF_INET);
    if (!he) {
        perror("gethostbyaddr");
        printf("%d\n", errno);
        return;
    }

    printf("name %s\n", he->h_name);

    for (int i = 0; he->h_aliases[i]; i++) {
        printf("aliases %s\n", he->h_aliases[i]);
    }

    printf("type = %s\n", he->h_addrtype == AF_INET ? "AF_INET" : "AF_INET6");

    for (int i = 0; he->h_addr_list[i]; i++) {
        printf("ip addr %s\n", inet_ntoa(*(struct in_addr *) he->h_addr_list[i]));
    }
}

#else

//基于windows的实现
//struct hostent* gethostbyname(const char *name);
//struct hostent* gethostbyaddr(const char *addr, int len, int type);

void windows_gethostbyname_test(const char *hostname) {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }
    struct hostent *he = gethostbyname(hostname);

    printf("name %s\n", he->h_name);

    for (int i = 0; he->h_aliases[i]; i++) {
        printf("aliases %s\n", he->h_aliases[i]);
    }

    printf("type = %s\n", he->h_addrtype == AF_INET ? "AF_INET" : "AF_INET6");

    for (int i = 0; he->h_addr_list[i]; i++) {
        printf("ip addr %s\n", inet_ntoa(*(struct in_addr *) he->h_addr_list[i]));
    }

    WSACleanup();
}

void windows_gethostbyaddr_test(const char *ip) {
    WSAData wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup error\n");
        return;
    }

    SOCKADDR_IN addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = inet_addr(ip);

    struct hostent *he = gethostbyaddr((char *) &addr.sin_addr, 4, AF_INET);
    if (!he) {
        printf("gethostbyaddr error %d\n", WSAGetLastError());
        return;
    }

    printf("name %s\n", he->h_name);

    for (int i = 0; he->h_aliases[i]; i++) {
        printf("aliases %s\n", he->h_aliases[i]);
    }

    printf("type = %s\n", he->h_addrtype == AF_INET ? "AF_INET" : "AF_INET6");

    for (int i = 0; he->h_addr_list[i]; i++) {
        printf("ip addr %s\n", inet_ntoa(*(struct in_addr *) he->h_addr_list[i]));
    }

    WSACleanup();
}

#endif

int main(int argc, char *argv[]) {
    //gethostbyname_test(argv[1]);

    //gethostbyaddr_test(argv[1]);

    //windows_gethostbyname_test(argv[1]);

    windows_gethostbyaddr_test(argv[1]);
    return 0;
}
