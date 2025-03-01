#include <iostream>

using namespace std;

//IPV4头部结构
// 4位版本号（值是4） + 4位头部长度（多少个4字节，最长60字节）+ 8位服务类型（3位忽略，最小延时，最大吞吐量，最高可靠性，最小费用，保留字段0） + 16位总长度（字节数，IP数据报最大为65535字节）
// 16位标识（同一个数据报所有分片具有相同的标识值） + 3位标志（保留字段0，DF位，MF位） + 13位片偏移（第个IP分片的长度是8的整数倍）
// 8位生存时间（常见值是64） + 8位协议（ICMP是1，TCP是6，UDP是17） + 16位头部校验和
// 32位源端IP地址
// 32位目的端IP地址
// 选项，最多40字节

//使用tcpdump观察IPV4头部结构
//tcpdump -ntx -i lo
//telnet 127.0.0.1

//IP 127.0.0.1.43932 > 127.0.0.1.telnet: Flags [S], seq 3037026495, win 65495, options [mss 65495,sackOK,TS val 1786941653 ecr 0,nop,wscale 7], length 0
//    0x0000:  4510 003c 44b0 4000 4006 f7f9 7f00 0001
//    0x0010:  7f00 0001 ab9c 0017 b505 58bf 0000 0000
//    0x0020:  a002 ffd7 fe30 0000 0204 ffd7 0402 080a
//    0x0030:  6a82 90d5 0000 0000 0103 0307

//0x4  IP版本号
//0x5  头部长度，5个4字节，20字节
//0x10 开启最小延时
//0x003c 数据报总长度60字节
//0x44b0 标识
//0x4    禁止分片
//0x000  分片偏移
//0x40   TTL为64
//0x06   表示上层协议是TCP
//0xf7f9 头部校验和
//0x7f000001 源IP地址127.0.0.1
//0x7f000001 目的IP地址127.0.0.1

//IP分片
//以太网帧的MTU是1500字节，它携带的IP数据报的数据部分最多是1480字节，头部占20字节。

//路由机制
//route

//IP转发
// /proc/sys/net/ipv4/ip_forward

//ICMP重定向
// /proc/sys/net/ipv4/conf/all/send_redirects  允许发送ICMP重定向报文
// /proc/sys/net/ipv4/conf/all/accept_redirects 允许接收ICMP重定向报文

//IPV6固定头部结构
// 4位版本号（值是6） + 8位通信类型（与IPV4中TOS类似） + 20位流标签
// 16位净荷长度（扩展头部和应用程序数据长度，不包固定头部长度） + 8位下一个包头（类似IPV4协议字段） + 8位跳数限制（IPV4中TTL）
// 128位源端IP地址
// 128位目的IP地址


int main() {
    return 0;
}
