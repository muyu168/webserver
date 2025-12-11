#include "socket/Socket.h"
#include <iostream>

Socket::Socket() : sockfd_(-1) {
    memset(&addr_,0,sizeof(addr_));
}

Socket::Socket(int sockfd) : sockfd_(sockfd){
    memset(&addr_,0,sizeof(addr_));
}

Socket::~Socket(){
    Close();
}

bool Socket::Create(){
    sockfd_ = socket(AF_INET,SOCK_STREAM,0);
    if(sockfd_ == -1){
        std::cerr << "create socket failed" << std::endl;
        return false;
    }
    return true;
}

bool Socket::Bind(const std::string& ip, int prot){
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(prot);
    if(ip.empty()){
        addr_.sin_addr.s_addr = INADDR_ANY;           //绑定所有网卡
    }else {
        addr_.sin_addr.s_addr = inet_addr(ip.c_str());
    }
    if(bind(sockfd_,(struct sockaddr*)&addr_,sizeof(addr_)) == -1){
        std::cerr << "bind socket failed" << std::endl;
        return false;
    }
    return true;
}

bool Socket::Listen(int backlog){
    if(listen(sockfd_,backlog) == -1){
        std::cerr << "listen socket failed" << std::endl;
        return false;
    }
    return true;
}

int Socket::Accept(struct sockaddr_in* client_addr){
    socklen_t len = sizeof(*client_addr);
    int client_sockfd = accept(sockfd_,(struct sockaddr*) client_addr,&len);
    if(client_sockfd == -1){
        std::cerr << "accept socket failed" << std::endl;
        return -1;
    }
    return client_sockfd;
}

int Socket::Connect(const char* ip, int port){
    socklen_t len = sizeof(addr_);
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port);
    addr_.sin_addr.s_addr = inet_addr(ip);
    if((connect(sockfd_,(struct sockaddr*)&addr_,len)) == -1){
        std::cerr << "connect socket failed" << std::endl;
        return -1;
    }
    return 0;
}

void Socket::Close(){
    if(sockfd_ != -1){
        close(sockfd_);
        sockfd_ = -1;
    }
}

bool Socket::SetNonBlocking(){
    //获取当前flags
    int flags = fcntl(sockfd_, F_GETFL, 0);
    //设置非阻塞
    flags |= O_NONBLOCK;
    if(fcntl(sockfd_, F_SETFL, flags) == -1){
        std::cerr << "set nonblocking failed" << std::endl;
        return false;
    }
    return true;
}

bool Socket::SetReuseAddr(){
    int opt = 1;
    if(setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1){
        std::cerr << "set reuseaddr failed" << std::endl;
        return false;
    }
    return true;
}

int Socket::Send(const void* data, int len){
    // send() 返回实际发送的字节数
    // 返回 -1 表示错误，返回 0 表示连接关闭
    int ret = send(sockfd_, data, len, 0);
    if(ret == -1){
        std::cerr << "send data failed" << std::endl;
    }
    return ret;
}

int Socket::Recv(void* data, int len){
    // recv() 返回实际接收的字节数
    // 返回 -1 表示错误，返回 0 表示连接关闭
    int ret = recv(sockfd_, data, len, 0);
    if(ret == -1){
        std::cerr << "recv data failed" << std::endl;
    }
    return ret;
}




