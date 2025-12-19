#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "socket/Socket.h"
#include "epoll/Epoll.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <string>
#include <map>

class WebServer {
public:
    WebServer(int port) : port_(port) {};
    ~WebServer() = default;
    bool Start();                                                               // 启动服务器       
    void Run();                                                                 // 运行事件循环
    void Stop();                                                                // 停止服务器

private:
    void HandleNewConnection();                                                 // 处理新连接请求   
    void HandleClientRequest(int client_fd);                                    // 处理客户端请求   
    void ProcessHttpRequest(int client_fd, const HttpRequest& request);     // 处理HTTP请求
    HttpResponse HandleRequest(const HttpRequest& request);                     // 返回响应

private:
    int port_;                                                                  // 监听端口
    Socket server_socket_;                                                      // 服务器套接字
    Epoll epoll_;                                                                // epoll对象        
    bool running_;                                                              // 服务器运行状态  
    std::map<int, std::string> client_buffer_;                                  // 客户端请求缓存
};

#endif