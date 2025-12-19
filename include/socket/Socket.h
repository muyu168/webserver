#ifndef SOCKET_H
#define SOCKET_H

#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

class Socket {
public:
    Socket();                                           //默认构造函数
    explicit Socket(int sockfd);                        //构造函数，传入 socket 文件描述符

    ~Socket();                                          //析构函数

    Socket(const Socket&) = delete;                     //禁止拷贝构造函数
    Socket& operator=(const Socket&) = delete;          //禁止拷贝赋值运算符

    bool Create();                                      //创建 socket
    bool Bind(const std::string& ip,int port);          //绑定 ip 地址和端口
    bool Listen(int backlog);                           //监听连接
    int  Accept(struct sockaddr_in* client_addr);       //接受连接
    int  Connect(const char* ip, int port);             //连接服务器

    int  Send(const void* data, int len);               //发送数据
    int  Recv(void* data, int len);                     //接收数据

    void Close();                                       //关闭 socket
    int  GetFd() const {return sockfd_;};               //获取 socket 文件描述符
    bool SetNonBlocking();                              //设置非阻塞模式
    bool SetReuseAddr();                                //设置地址复用

private:
    int sockfd_;                                        //socket 文件描述符 
    struct sockaddr_in addr_;                           //socket 地址结构体
};




#endif