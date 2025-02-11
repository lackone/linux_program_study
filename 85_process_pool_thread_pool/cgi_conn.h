#ifndef CGI_CONN_H
#define CGI_CONN_H

#include <cstring>

#include "process_pool.h"

class cgi_conn {
public:
    cgi_conn() {
    }

    ~cgi_conn() {
    }

    //初始化客户端
    void init(int epfd, int fd, const sockaddr_in &addr) {
        _epfd = epfd;
        _fd = fd;
        _addr = addr;
        memset(_buf, 0, BUFFER_SIZE);
        _read_idx = 0;
    }

    void process() {
        int idx = 0;
        int ret = -1;

        while (1) {
            idx = _read_idx;
            ret = recv(_fd, _buf + idx, BUFFER_SIZE - 1 - idx, 0);
            if (ret < 0) {
                if (errno != EAGAIN) {
                    remove_fd(_epfd, _fd);
                }
                break;
            } else if (ret == 0) {
                remove_fd(_epfd, _fd);
                break;
            } else {
                _read_idx += ret;

                printf("user data %s\n", _buf);

                for (; idx < _read_idx; ++idx) {
                    if ((idx >= 1) && (_buf[idx - 1] == '\r') && (_buf[idx] == '\n')) {
                        break;
                    }
                }

                //如果没有遇到\r\n，则需要读更多数据
                if (idx == _read_idx) {
                    continue;
                }

                _buf[idx - 1] = '\0';

                char *file_name = _buf;
                //判断要运行的CGI程序是否存在
                if (access(file_name, F_OK) == -1) {
                    remove_fd(_epfd, _fd);
                    break;
                }
                //创建子进程来执行CGI程序
                ret = fork();
                if (ret == -1) {
                    remove_fd(_epfd, _fd);
                    break;
                } else if (ret > 0) {
                    remove_fd(_epfd, _fd);
                    break;
                } else {
                    //子进程将标准输出重定向到sock，并执行cgi程序
                    close(STDOUT_FILENO);
                    dup(_fd);
                    execl(_buf, _buf, NULL);
                    exit(0);
                }
            }
        }
    }

private:
    static const int BUFFER_SIZE = 1024;
    static int _epfd;
    int _fd;
    sockaddr_in _addr;
    char _buf[BUFFER_SIZE]; //缓冲
    int _read_idx; //保存读缓冲中已经读入的客户数据的最后一个字节的下一个位置
};

int cgi_conn::_epfd = -1;

#endif //CGI_CONN_H
