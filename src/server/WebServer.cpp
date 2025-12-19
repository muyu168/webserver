#include "server/WebServer.h"
#include <iostream>
#include <arpa/inet.h>

#define INET_ADDRSTRLEN 16
// 启动服务器 
bool WebServer::Start(){
    std::cout << "WebServer 启动" << std::endl;
    server_socket_.Create();
    server_socket_.SetNonBlocking();                                                // 设置非阻塞模式                                                         // 创建服务器套接字
    server_socket_.SetReuseAddr();                                                  // 设置 SO_REUSEADDR 选项
    server_socket_.Bind("",port_);                                                     // 绑定端口
    epoll_.Add(server_socket_.GetFd(), EPOLLIN | EPOLLET);                          // 注册服务器套接字到 epoll
    running_ = true;                                                                // 服务器运行状态
    server_socket_.Listen(128);                                                     // 监听端口
    std::cout << "WebServer 启动成功" << std::endl;
    Run();
    return true;
}

// 运行事件循环
void WebServer::Run(){
    while(running_){
        int num_events = epoll_.Wait(-1);
        for(int i = 0 ; i < num_events ; i++){
            int client_fd = epoll_.GetEventsFd(i);
            if(client_fd == server_socket_.GetFd()){
                // 处理新连接请求
                HandleNewConnection();
            }else{
                // 处理客户端请求
                HandleClientRequest(client_fd);
            }
        }
    }
}

// 停止服务器
void WebServer::Stop(){
    running_ = false;
    epoll_.Delete(server_socket_.GetFd());
    server_socket_.Close();
    std::cout << "WebServer 停止" << std::endl;
}

// 处理新连接请求   
void WebServer::HandleNewConnection(){
    struct sockaddr_in client_addr;
    int client_fd_ = server_socket_.Accept(&client_addr);
    if(client_fd_ < 0){
        std::cout << "Accept 失败" << std::endl;
        return;
    }
    epoll_.Add(client_fd_, EPOLLIN | EPOLLET);
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    std::cout << "新连接来自 " << ip_str << "端口:" << ntohs(client_addr.sin_port) << std::endl;
}

// 处理客户端请求   
void WebServer::HandleClientRequest(int client_fd){
    char buf[4096];
    int n = recv(client_fd, buf, sizeof(buf), 0);
    if(n > 0){
        client_buffer_[client_fd] += std::string(buf,n);
        if(client_buffer_[client_fd].size() > 4096){
            // 请求过大，关闭连接
            std::cout << "请求过大，关闭连接" << std::endl;
            epoll_.Delete(client_fd);
            close(client_fd);
            client_buffer_.erase(client_fd);
            return;
        }
        // 解析请求
        HttpRequest request;
        if(HttpRequest::IsComplete(client_buffer_[client_fd])){
            // 请求完整，处理请求
            if(request.Parse(client_buffer_[client_fd])){
                // 处理HTTP请求
                ProcessHttpRequest(client_fd, request);
            }else{
                // 解析请求失败，关闭连接
                std::cout << "解析请求失败，关闭连接" << std::endl;
                epoll_.Delete(client_fd);
                close(client_fd);
            }
        }else{
            // 请求不完整
            std::cout << "请求不完整" << std::endl;
            epoll_.Delete(client_fd);
            close(client_fd);
            client_buffer_.erase(client_fd);
            return;
        }
        
    }
}     

// 处理HTTP请求
void WebServer::ProcessHttpRequest(int client_fd, const HttpRequest& request){
     HttpResponse response = HandleRequest(request);
            // 发送响应
            std::string response_str = response.GetResponse();
            send(client_fd, response_str.c_str(), response_str.size(), 0);
            client_buffer_.erase(client_fd);
            epoll_.Delete(client_fd);
            close(client_fd);
}

// 返回响应
HttpResponse WebServer::HandleRequest(const HttpRequest& request){
    HttpResponse response;
    response.SetStatusCode(200);
    response.SetStatusMessage("OK");
    response.AddHeader("Content-Type", "text/html");
    response.SetBody("<html><body><h1>Hello, World!</h1></body></html>");
    return response;
}                   
