#include "epoll/Epoll.h"
#include "socket/Socket.h"
#include <iostream>

int main() {
    std::cout << "===Epoll测试程序===" << std::endl;
    Epoll epoll;
    std::cout << "Epoll创建成功" << std::endl;

    Socket Serversock;
    if(Serversock.Create()){
        std::cout << "Serversock创建成功" << std::endl;
    }else{
        std::cout << "Serversock创建失败" << std::endl;
    }

    if(epoll.Add(Serversock.GetFd(), EPOLLIN)){
        std::cout << "Serversock添加EPOLLIN事件到Epoll成功" << std::endl;
    }else{
        std::cout << "Serversock添加EPOLLIN事件到Epoll失败" << std::endl;
    }

    int n = epoll.Wait(1000);  // 等待 1 秒
    if(n >= 0){
        std::cout << "Epoll等待成功，就绪事件数量: " << n << std::endl;
    }else{
        std::cout << "Epoll等待失败" << std::endl;
    }

    std::cout << "===Epoll测试程序结束===" << std::endl;

    return 0;
}