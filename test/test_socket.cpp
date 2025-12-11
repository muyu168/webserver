#include "socket/Socket.h"
#include <iostream>

int main(){

    std::cout << "===Socket测试程序===" << std::endl;

    Socket sock;

    if(sock.Create()){
        std::cout << "创建Socket成功" << std::endl;
    }else {
        std::cout << "创建Socket失败" << std::endl;
    }

    if(sock.SetReuseAddr()){
        std::cout << "设置SO_REUSEADDR成功" << std::endl;
    }else {
        std::cout << "设置SO_REUSEADDR失败" << std::endl;
    }

    if(sock.Bind("", 8080)){
        std::cout << "绑定端口成功" << std::endl;
    }else {
        std::cout << "绑定端口失败" << std::endl;
    }

    if(sock.Listen(10)){
        std::cout << "监听成功" << std::endl;
    }else {
        std::cout << "监听失败" << std::endl;
    }

    std::cout << "===Socket测试程序结束===" << std::endl;

    return 0;
}