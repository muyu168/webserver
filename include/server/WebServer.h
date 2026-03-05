#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "timer/Timer.h"
#include "http/MimeType.h"
#include "pool/ThreadPool.h"
#include "base/Logger.h"
#include "socket/Socket.h"
#include "epoll/Epoll.h"
#include "http/HttpRequest.h"
#include "http/HttpRequestFSM.h"
#include "http/HttpResponse.h"
#include <string>
#include <map>
#include <sys/stat.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <queue>
#include <mutex>

struct PendingResponse{
    int client_fd;  // 修复拼写错误：clinet_fd -> client_fd
    std::string data;
    bool keep_alive;  // 是否保持连接
};

class WebServer {
public:
    WebServer(int port, const std::string& root_path = "./www") : port_(port) , running_(false), root_path_(root_path){};
    ~WebServer() = default;
    bool Start();                                                               // 启动服务器       
    void Run();                                                                 // 运行事件循环

private:
    void HandleSignal(int signal_fd);                                                        // 处理信号
    void Stop();                                                                // 停止服务器
    bool SetNoBlocking(int fd);                                                // 设置非阻塞模式
    void CloseConnection(int client_fd);
    void HandleNewConnection();                                                 // 处理新连接请求   
    void HandleClientRequest(int client_fd);                                    // 处理客户端请求   
    void parseRequest(int client_fd, HttpRequest& request, const std::string& data);               // 解析HTTP请求
    void SendResponse(PendingResponse& to_response);                               // 返回HTTP响应
    HttpResponse HandleRequest(const HttpRequestFSM& request);                     // 返回响应
    HttpResponse HandlePostRequest(const HttpRequestFSM& request);                 // 处理POST请求
    void ProcessPendingResponse();

private:
    int signal_fd_;                                                             // 信号文件描述符
    int event_fd_;                                                              // 事件文件描述符
    int port_;                                                                  // 监听端口
    ThreadPool thread_pool_;                                                    // 线程池对象
    Socket server_socket_;                                                      // 服务器套接字
    Timer timer_;                                                               // 定时器对象
    Epoll epoll_;                                                               // epoll对象
    bool running_;                                                              // 运行状态
    std::string root_path_;

    // 线程安全的数据结构（需要加锁保护）
    std::mutex buffer_mutex_;                                                   // 缓冲区互斥锁
    std::unordered_map<int, std::string> client_buffer_;                        // 客户端请求缓存

    std::mutex fsm_mutex_;                                                      // 状态机互斥锁
    std::unordered_map<int, HttpRequestFSM> request_fsm_;                       // 请求状态机

    std::mutex response_mutex_;                                                 // 响应队列互斥锁
    std::queue<PendingResponse> response_queue_;                                // 待发送响应队列
};

#endif