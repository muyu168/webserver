#include "socket/Socket.h"
#include "epoll/Epoll.h"
#include <iostream>
#include <cstring>

int main(int argc, char* argv[]){
    if(argc!= 2){
        std::cout << "Usage: " << argv[0] << " port :" << std::endl;
        return 1;
    }
    int port = atoi(argv[1]);
    Socket server_sock;
    if(!server_sock.Create()){
        std::cout << "创建socket失败" << std::endl;
        return 1;
    }
    if(!server_sock.SetReuseAddr()){
        std::cout << "设置地址复用失败" << std::endl;
        return 1;
    }
    if(!server_sock.Bind("", port)){
        std::cout << "绑定端口失败" << std::endl;
        return 1;
    }
    if(!server_sock.Listen(128)){
        std::cout << "监听失败" << std::endl;
        return 1;
    }
    std::cout << "服务器监听在 " << port << " 端口" << std::endl;
    Epoll epoll;
    epoll.Add(server_sock.GetFd(), EPOLLIN);

    while(true){
        int n = epoll.Wait(-1);  // 永久等待
        if(n == -1){
            std::cerr << "epoll wait 失败" << std::endl;
            break;
        }

        for(int i = 0; i < n; i++){  // 只遍历就绪的事件
            int fd = epoll.GetEventsFd(i);
            uint32_t events = epoll.GetEvents(i);
            if(fd == server_sock.GetFd()){
                struct sockaddr_in clientaddr;
                int clientfd = server_sock.Accept(&clientaddr);
                if(clientfd == -1){
                    std::cout << "接受客户端连接失败" << std::endl;
                    continue;
                }
                std::cout << "客户端连接成功" << std::endl;
                epoll.Add(clientfd, EPOLLIN);
            }else if(events & EPOLLIN){
                char readbuf[128];
                int len = sizeof(readbuf);
                int n = recv(fd, readbuf, len, 0);
                if(n == -1){
                    std::cout << "接收数据失败" << std::endl;
                    epoll.Delete(fd);
                }else if(n == 0){
                    std::cout << "客户端断开连接" << std::endl;
                    epoll.Delete(fd);
                    close(fd);
                }else{
                    readbuf[n] = '\0';  // 添加字符串结束符
                    std::cout << "收到数据：" << readbuf << std::endl;
                    send(fd, readbuf, n, 0);
                }
            }
        }
    }



    return 0;
}