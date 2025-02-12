#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <stdarg.h>

#include "thread_pool.h"

const char *ok_200_title = "ok";
const char *error_400_title = "bad request";
const char *error_400_form = "you request has bad syntax or is inherently impossible to satisfy.\n";
const char *error_403_title = "forbidden";
const char *error_403_form = "you do not hav permission to get file from this server.\n";
const char *error_404_title = "not found";
const char *error_404_form = "the requested file was not found on this server.\n";
const char *error_500_title = "internal error";
const char *error_500_form = "there was an unusual problem serving the requested file.\n";

const char *doc_root = "/data/wwwroot/html";

int setnonblocking2(int fd) {
    int old_opt = fcntl(fd, F_GETFL);
    int new_opt = old_opt | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_opt);
    return old_opt;
}

void add_fd2(int epfd, int fd, bool one_shot) {
    epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot) {
        ev.events |= EPOLLONESHOT;
    }
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    setnonblocking2(fd);
}

void remove_fd2(int epfd, int fd) {
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}

void mod_fd2(int epfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

class http_conn {
public:
    static const int FILE_NAME_LEN = 200; //文件名最大长度
    static const int READ_BUF_SIZE = 2048; //读缓冲大小
    static const int WRITE_BUF_SIZE = 1024; //写缓冲大小
    enum METHOD { GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH }; //HTTP请求方法
    enum CHECK_STATE { CHECK_STATE_REQUEST_LINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT }; //解析客户请求
    enum HTTP_CODE {
        NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR,
        CLOSED_CONNECTION
    }; //服务器处理HTTP请求的结果
    enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN }; //行的读取状态
public:
    http_conn() {
    }

    ~http_conn() {
    }

    //初始化新连接
    void init(int fd, const sockaddr_in &addr) {
        _fd = fd;
        _addr = addr;

        //避免TIME_WAIT状态，用于调试
        int reuse = 1;
        setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        add_fd2(_epfd, _fd, true);
        _user_cnt++;

        init();
    }

    //关闭连接
    void close_conn(bool real_close = true) {
        if (real_close && _fd != -1) {
            remove_fd2(_epfd, _fd);
            _fd = -1;
            _user_cnt--; //将客户数量减1
        }
    }

    //处理客户请求
    void process() {
        HTTP_CODE ret = process_read();
        if (ret == NO_REQUEST) {
            mod_fd2(_epfd, _fd, EPOLLIN); //继续监听读
            return;
        }
        bool w_ret = process_write(ret);
        if (!w_ret) {
            close_conn();
        }

        mod_fd2(_epfd, _fd, EPOLLOUT);
    }

    //非阻塞读
    bool read() {
        if (_read_idx >= READ_BUF_SIZE) {
            return false;
        }

        //循环读取数据，直接无数据可读 或 对方关闭连接
        int bytes_read = 0;
        while (1) {
            bytes_read = recv(_fd, _read_buf + _read_idx, READ_BUF_SIZE - _read_idx, 0);
            if (bytes_read == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                return false;
            } else if (bytes_read == 0) {
                return false;
            }

            _read_idx += bytes_read;
        }

        return true;
    }

    //非阻塞写
    //写http响应
    bool write() {
        int tmp = 0;
        int bytes_have_send = 0; //已经发送的数据
        int bytes_to_send = _write_idx; //剩余要发送的数据

        if (bytes_to_send == 0) {
            //要发送的数据为0
            mod_fd2(_epfd, _fd, EPOLLIN); //重新监听读
            init();
            return true;
        }

        while (1) {
            tmp = writev(_fd, _iov, _iov_cnt);
            if (tmp <= -1) {
                //如果写缓冲没有空间，等待下轮EPOLLOUT事件
                if (errno == EAGAIN) {
                    mod_fd2(_epfd, _fd, EPOLLOUT);
                    return true;
                }
                unmap();
                return false;
            }
            bytes_to_send -= tmp;
            bytes_have_send += tmp;
            if (bytes_to_send <= bytes_have_send) {
                unmap();
                //根据http请求中的connection字段决定是否立即关闭连接
                if (_linger) {
                    init();
                    mod_fd2(_epfd, _fd, EPOLLIN);
                    return true;
                } else {
                    mod_fd2(_epfd, _fd, EPOLLIN);
                    return false;
                }
            }
        }
    }

private:
    //初始化连接
    void init() {
        _chk_state = CHECK_STATE_REQUEST_LINE;
        _linger = false;

        _method = GET;
        _url = 0;
        _version = 0;
        _content_length = 0;
        _host = 0;
        _start_line = 0;
        _chk_idx = 0;
        _read_idx = 0;
        _write_idx = 0;

        memset(_read_buf, 0, READ_BUF_SIZE);
        memset(_write_buf, 0, WRITE_BUF_SIZE);
        memset(_real_file, 0, FILE_NAME_LEN);
    }

    //解析HTTP请求
    HTTP_CODE process_read() {
        LINE_STATUS line_status = LINE_OK;
        HTTP_CODE ret = NO_REQUEST;
        char *txt = 0;

        while (((_chk_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) ||
               ((line_status = parse_line()) == LINE_OK)
        ) {
            txt = get_line();
            _start_line = _chk_idx;
            printf("got http line %s\n", txt);

            switch (_chk_state) {
                case CHECK_STATE_REQUEST_LINE: {
                    ret = parse_request_line(txt);
                    if (ret == BAD_REQUEST) {
                        return BAD_REQUEST;
                    }
                    break;
                }
                case CHECK_STATE_HEADER: {
                    ret = parse_headers(txt);
                    if (ret == BAD_REQUEST) {
                        return BAD_REQUEST;
                    } else if (ret == GET_REQUEST) {
                        return do_request();
                    }
                    break;
                }
                case CHECK_STATE_CONTENT: {
                    ret = parse_content(txt);
                    if (ret == GET_REQUEST) {
                        return do_request();
                    }
                    line_status = LINE_OPEN;
                    break;
                }
                default: {
                    return INTERNAL_ERROR;
                }
            }
        }

        return NO_REQUEST;
    }

    //填充HTTP应答
    bool process_write(HTTP_CODE ret) {
        switch (ret) {
            case INTERNAL_ERROR: {
                add_status_line(500, error_500_title);
                add_headers(strlen(error_500_form));
                if (!add_content(error_500_form)) {
                    return false;
                }
                break;
            }
            case BAD_REQUEST: {
                add_status_line(400, error_400_title);
                add_headers(strlen(error_400_form));
                if (!add_content(error_400_form)) {
                    return false;
                }
                break;
            }
            case NO_RESOURCE: {
                add_status_line(404, error_404_title);
                add_headers(strlen(error_404_form));
                if (!add_content(error_404_form)) {
                    return false;
                }
                break;
            }
            case FORBIDDEN_REQUEST: {
                add_status_line(403, error_403_title);
                add_headers(strlen(error_403_form));
                if (!add_content(error_403_form)) {
                    return false;
                }
                break;
            }
            case FILE_REQUEST: {
                add_status_line(200, ok_200_title);
                if (_file_stat.st_size != 0) {
                    add_headers(_file_stat.st_size);
                    _iov[0].iov_base = _write_buf;
                    _iov[0].iov_len = _write_idx;
                    _iov[1].iov_base = _file_addr;
                    _iov[1].iov_len = _file_stat.st_size;
                    _iov_cnt = 2;
                    return true;
                } else {
                    const char *ok_string = "<html><body></body></html>";
                    add_headers(strlen(ok_string));
                    if (!add_content(ok_string)) {
                        return false;
                    }
                }
            }
            default: {
                return false;
            }
        }

        _iov[0].iov_base = _write_buf;
        _iov[0].iov_len = _write_idx;
        _iov_cnt = 1;
        return true;
    }

    //下面一组函数被process_read调用分析http请求
    HTTP_CODE parse_request_line(char *txt) {
        _url = strpbrk(txt, " \t");
        if (!_url) {
            return BAD_REQUEST;
        }
        *_url++ = '\0';
        char *method = txt;
        if (strcasecmp(method, "GET") == 0) {
            _method = GET;
        } else {
            return BAD_REQUEST;
        }
        _url += strspn(_url, " \t"); //查找url中不在" \t"中出现的字符下标，相当于清空url前面的空白字符
        _version = strpbrk(_url, " \t");
        if (!_version) {
            return BAD_REQUEST;
        }
        *_version++ = '\0';
        _version += strspn(_version, " \t");
        if (strcasecmp(_version, "HTTP/1.1") != 0) {
            return BAD_REQUEST;
        }
        if (strncasecmp(_url, "http://", 7) == 0) {
            _url += 7;
            _url = strchr(_url, '/');
        }
        if (!_url || _url[0] != '/') {
            return BAD_REQUEST;
        }
        _chk_state = CHECK_STATE_HEADER;
        return NO_REQUEST;
    }

    HTTP_CODE parse_headers(char *txt) {
        if (txt[0] == '\0') {
            //遇到空行，表示头部字段解析完毕
            //如果http有消息体，还需要读取_content_length字节的消息体，状态机转移到CHECK_STATE_CONTENT
            if (_content_length != 0) {
                _chk_state = CHECK_STATE_CONTENT;
                return NO_REQUEST;
            }

            //我们已经得到一个完整的HTTP请求
            return GET_REQUEST;
        } else if (strncasecmp(txt, "Connection:", 11) == 0) {
            txt += 11;
            txt += strspn(txt, " \t");
            if (strcasecmp(txt, "keep-alive") == 0) {
                _linger = true; //保持连接
            }
        } else if (strncasecmp(txt, "Content-Length:", 15) == 0) {
            txt += 15;
            txt += strspn(txt, " \t");
            _content_length = atol(txt); //获取内容长度
        } else if (strncasecmp(txt, "Host:", 5) == 0) {
            txt += 5;
            txt += strspn(txt, " \t");
            _host = txt;
        } else {
            printf("oop! unknow header %s\n", txt);
        }

        return NO_REQUEST;
    }

    HTTP_CODE parse_content(char *txt) {
        //只是判断是否被完整地读入了
        if (_read_idx >= (_content_length + _chk_idx)) {
            txt[_content_length] = '\0';
            return GET_REQUEST;
        }
        return NO_REQUEST;
    }

    HTTP_CODE do_request() {
        strcpy(_real_file, doc_root);
        int len = strlen(doc_root);
        strncpy(_real_file + len, _url, FILE_NAME_LEN - len - 1);
        if (stat(_real_file, &_file_stat) < 0) {
            return NO_RESOURCE;
        }
        if (!(_file_stat.st_mode & S_IROTH)) {
            //其他用户有读权限
            return FORBIDDEN_REQUEST;
        }
        if (S_ISDIR(_file_stat.st_mode)) {
            //是否目录
            return BAD_REQUEST;
        }
        int fd = open(_real_file, O_RDONLY);
        _file_addr = (char *) mmap(NULL, _file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        return FILE_REQUEST;
    }

    char *get_line() {
        return _read_buf + _start_line;
    }

    LINE_STATUS parse_line() {
        char tmp;
        //_chk_idx当前分析的字符下标
        //_read_idx读取用户数据的长度
        for (; _chk_idx < _read_idx; ++_chk_idx) {
            tmp = _read_buf[_chk_idx];
            if (tmp == '\r') {
                if ((_chk_idx + 1) == _read_idx) {
                    //如果下一个字符就是结尾，则说明没有读完
                    return LINE_OPEN;
                } else if (_read_buf[_chk_idx + 1] == '\n') {
                    //如果下一个字符是\n，说明已经完整读取到一行
                    _read_buf[_chk_idx++] = '\0';
                    _read_buf[_chk_idx++] = '\0';
                    return LINE_OK;
                }
                return LINE_BAD;
            } else if (tmp == '\n') {
                if ((_chk_idx > 1) && (_read_buf[_chk_idx - 1] == '\r')) {
                    _read_buf[_chk_idx - 1] = '\0';
                    _read_buf[_chk_idx++] = '\0';
                    return LINE_OK;
                }
                return LINE_BAD;
            }
        }

        //如果所有内容分析完毕，没有遇到\r，说明还要继续读取客户端数据
        return LINE_OPEN;
    }

    //下面一组被process_write函数填充http应答
    void unmap() {
        if (_file_addr) {
            munmap(_file_addr, _file_stat.st_size);
            _file_addr = NULL;
        }
    }

    bool add_response(const char *format, ...) {
        if (_write_idx >= WRITE_BUF_SIZE) {
            return false;
        }
        va_list al;
        va_start(al, format);
        int len = vsnprintf(_write_buf + _write_idx, WRITE_BUF_SIZE - 1 - _write_idx, format, al);
        if (len >= (WRITE_BUF_SIZE - 1 - _write_idx)) {
            return false;
        }
        _write_idx += len;
        va_end(al);
        return true;
    }

    bool add_content(const char *content) {
        return add_response("%s", content);
    }

    bool add_status_line(int status, const char *title) {
        return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
    }

    bool add_headers(int content_length) {
        add_content_length(content_length);
        add_linger();
        add_blank_line();
        return true;
    }

    bool add_content_length(int content_length) {
        return add_response("Content-Length: %d\r\n", content_length);
    }

    bool add_linger() {
        return add_response("Connection: %s\r\n", (_linger ? "keep-alive" : "close"));
    }

    bool add_blank_line() {
        return add_response("%s", "\r\n");
    }

public:
    //所有socket上的事件，都被注册到同一个epoll内核事件表中
    static int _epfd;
    static int _user_cnt; //用户数量
private:
    int _fd; //http连接的socket
    sockaddr_in _addr; //socket的地址

    char _read_buf[READ_BUF_SIZE]; //读缓冲
    int _read_idx; //标识读缓冲中已经读入的客户数据的最后一个字节的下一个位置
    int _chk_idx; //当前正在分析的字符在缓冲中的位置
    int _start_line; //当前解析的行的超始位置

    char _write_buf[WRITE_BUF_SIZE];
    int _write_idx; //写缓冲中待发送的字节数

    CHECK_STATE _chk_state; //当前所处的状态
    METHOD _method; //方法

    char _real_file[FILE_NAME_LEN]; //客户请求的目标文件的完整路径
    char *_url; //客户请求的目标文件的文件名
    char *_version; //http协议版本号
    char *_host; //主机名
    int _content_length; //消息体的长度
    bool _linger; //是否要求保持连接

    char *_file_addr; //客户请求的目标文件被mmap到内存中的起始位置
    struct stat _file_stat; //目标文件状态

    //采用writev来执行写操作
    struct iovec _iov[2];
    int _iov_cnt;
};

int http_conn::_epfd = -1;
int http_conn::_user_cnt = 0;

#endif //HTTP_CONN_H
