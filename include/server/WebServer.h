#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "pool/ThreadPool.h"
#include "base/Logger.h"
#include "socket/Socket.h"
#include "epoll/Epoll.h"
#include "http/HttpRequest.h"
#include "http/HttpResponse.h"
#include <string>
#include <map>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <queue>
#include <mutex>

struct PendingResponse{
    int clinet_fd;
    std::string data;
};

class WebServer {
public:
    WebServer(int port) : port_(port) , running_(false){};
    ~WebServer() = default;
    bool Start();                                                               // 启动服务器       
    void Run();                                                                 // 运行事件循环
    void Stop();                                                                // 停止服务器


private:
    bool SetNoBlocking(int fd);                                                // 设置非阻塞模式
    void ColseConnection(int client_fd);
    void HandleNewConnection();                                                 // 处理新连接请求   
    void HandleClientRequest(int client_fd);                                    // 处理客户端请求   
    void parseRequest(int client_fd, HttpRequest& request, const std::string& data);               // 解析HTTP请求
    void SendResponse(int client_fd, const std::string& to_response);           // 返回HTTP响应
    HttpResponse HandleRequest(const HttpRequest& request);                     // 返回响应
    void ProcessPendingResponse();

private:
    int event_fd_;
    int port_;                                                                  // 监听端口
    ThreadPool thread_pool_;
    Socket server_socket_;                                                      // 服务器套接字
    Epoll epoll_;                                                                // epoll对象        
    bool running_;                                                              // 运行状态
    std::mutex response_mutex_;                                                 // 响应队列互斥锁
    std::queue<PendingResponse> response_queue_;                                // 待发送响应队列
    std::map<int, std::string> client_buffer_;                                  // 客户端请求缓存
};

#endif