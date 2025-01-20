#include <iostream>
#include <cstring>
#include <io.h>
#if defined(__linux__) || defined(__unix__)
#include <unistd.h>
#include <netinet/in.h>
#include <fcntl.h>
#else
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace std;

int main() {

    return 0;
}