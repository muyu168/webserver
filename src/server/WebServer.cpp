#include "server/WebServer.h"
#include <iostream>
#include <arpa/inet.h>

#define INET_ADDRSTRLEN 16
// 启动服务器 
bool WebServer::Start(){
    Logger::Instance().Init("./logs/webserver.log", LogLevel::INFO);
    LOG_INFO("WebServer 启动");
    server_socket_.Create();
    server_socket_.SetNonBlocking();                                                // 设置非阻塞模式                                                         // 创建服务器套接字
    server_socket_.SetReuseAddr();                                                  // 设置 SO_REUSEADDR 选项
    server_socket_.Bind("",port_);                                                     // 绑定端口
    epoll_.Add(server_socket_.GetFd(), EPOLLIN | EPOLLET);                          // 注册服务器套接字到 epoll
    running_ = true;                                                                // 服务器运行状态
    server_socket_.Listen(128);                                                     // 监听端口
    LOG_INFO("WebServer 启动成功");
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
    LOG_INFO("WebServer 停止");
}

// 处理新连接请求   
void WebServer::HandleNewConnection(){
    while(true){
        struct sockaddr_in client_addr;
        int client_fd_ = server_socket_.Accept(&client_addr);
        if(client_fd_ > 0){
            epoll_.Add(client_fd_, EPOLLIN | EPOLLET);
            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
            LOG_INFO("新连接来自%s , 端口：%d",ip_str,ntohs(client_addr.sin_port));
        }else {
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                break;
            }
            if(client_fd_ < 0){
            LOG_ERROR("Accept 失败");
            return;
            }
        }

    }  
    
}

// 处理客户端请求   
void WebServer::HandleClientRequest(int client_fd){
    char buf[4096];
    while(true){
        int n = recv(client_fd, buf, sizeof(buf), 0);
        if(n > 0){
            client_buffer_[client_fd] += std::string(buf,n);
            if(client_buffer_[client_fd].size() > 4096){
                // 请求过大，关闭连接
                LOG_ERROR("%d请求过大，关闭连接", client_fd);
                ColseConnection(client_fd);
                return;
            }
            // 解析请求
            HttpRequest request;
            if(HttpRequest::IsComplete(client_buffer_[client_fd])){
                // 请求完整，处理请求
                if(request.Parse(client_buffer_[client_fd])){
                    // 处理HTTP请求
                    ProcessHttpRequest(client_fd, request);
                    client_buffer_[client_fd].clear();
                }else{
                    // 解析请求失败，关闭连接
                    LOG_ERROR("解析请求失败，关闭连接");
                    ColseConnection(client_fd);
                }
            }else{
                // 请求不完整
                LOG_INFO("请求不完整");
                ColseConnection(client_fd);
                return;
            }
        }else if(n == 0){
            //对端关闭连接
            LOG_INFO("客户端关闭连接");
            ColseConnection(client_fd);
            return;
        }else {
            //读取出错
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                //数据读完了，等待下一次事件
                break;
            }else {
                LOG_ERROR("recv错误:%s",strerror(errno));
                ColseConnection(client_fd);
                return;
            }

        }
    }
}     

// 处理HTTP请求
void WebServer::ProcessHttpRequest(int client_fd, const HttpRequest& request){
    HttpResponse response = HandleRequest(request);
    // 发送响应
    std::string response_str = response.GetResponse();
    size_t send_len = 0;
    size_t to_send = response_str.size();
    while(to_send > send_len){
        ssize_t len = send(client_fd, response_str.c_str() + send_len, to_send - send_len, 0);
        if(len < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                
            }else {
                LOG_ERROR("send错误:%s", strerror(errno));
            }
            break;
        }
        send_len += len;
    }

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

//关闭连接
void WebServer::ColseConnection(int client_fd){
        epoll_.Delete(client_fd);
        close(client_fd);
        client_buffer_.erase(client_fd);
}