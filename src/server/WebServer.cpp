#include "server/WebServer.h"
#include <chrono>
#include <iostream>
#include <arpa/inet.h>

#define INET_ADDRSTRLEN 16
#define TIMEOUT_MS 5000
// 启动服务器 
bool WebServer::Start(){
    LOG_INFO("WebServer 启动");
    server_socket_.Create();
    server_socket_.SetNonBlocking();                                                // 设置非阻塞模式                                                         // 创建服务器套接字
    server_socket_.SetReuseAddr();                                                  // 设置 SO_REUSEADDR 选项
    server_socket_.Bind("",port_);                                                      // 绑定端口
    epoll_.Add(server_socket_.GetFd(), EPOLLIN | EPOLLET);                          // 注册服务器套接字到 epoll
    event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); 
    epoll_.Add(event_fd_, EPOLLIN | EPOLLET); 
    running_ = true;                                                                // 服务器运行状态
    server_socket_.Listen(128);                                                     // 监听端口
    LOG_INFO("WebServer 启动成功");
    Run();
    return true;
}

//设置非阻塞模式
bool WebServer::SetNoBlocking(int fd){
    //获取当前flags
    int flags = fcntl(fd, F_GETFL, 0);
    //设置非阻塞
    flags |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flags) == -1){
        return false;
    }
    return true;
}

// 运行事件循环
void WebServer::Run(){
    while(running_){
        int timeout = timer_.GetNextTimeout();
        int num_events = epoll_.Wait(timeout);
        for(int i = 0 ; i < num_events ; i++){
            int client_fd = epoll_.GetEventsFd(i);
            if(client_fd == server_socket_.GetFd()){
                // 处理新连接请求
                HandleNewConnection();
            }else if(client_fd == event_fd_){
                uint64_t value;
                eventfd_read(event_fd_, &value);
                std::queue<PendingResponse> to_response_queue;
                // 处理事件通知
                {
                    std::unique_lock<std::mutex> lock(response_mutex_);
                    to_response_queue.swap(response_queue_);
                }
                // 处理响应
                
                while(!to_response_queue.empty()){
                    PendingResponse response = std::move(to_response_queue.front());
                    to_response_queue.pop();
                    SendResponse(response);
                }
            }else{
                // 处理客户端请求
                HandleClientRequest(client_fd);
            }
        }
        //处理定时器事件
        timer_.Tick();
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
            SetNoBlocking(client_fd_);
            epoll_.Add(client_fd_, EPOLLIN | EPOLLET);
            //为新连接添加定时器
            auto ms = std::chrono::milliseconds(TIMEOUT_MS);
            timer_.AddTimer(client_fd_, ms, [this,client_fd = client_fd_](){
                ColseConnection(client_fd);
            });
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
            //收到客户端事件，调整定时器
                auto ms = std::chrono::milliseconds(TIMEOUT_MS);
                timer_.AdjustTimer(client_fd, ms, [this,fd = client_fd](){
                    ColseConnection(fd);
                });
            // 解析请求
            HttpRequest request;
            if(HttpRequest::IsComplete(client_buffer_[client_fd])){
                // 请求完整，处理请求
                std::string data = std::move(client_buffer_[client_fd]);
                thread_pool_.Enqueue([this, client_fd, data = std::move(data)](){
                    // 解析请求
                    HttpRequest request;
                    parseRequest(client_fd, request, data);
                    //生成响应
                    HttpResponse response = HandleRequest(request);
                    PendingResponse pending_reponse{client_fd, response.GetResponse()};
                    // 保存响应
                    {
                        std::lock_guard<std::mutex> lock(response_mutex_);
                        response_queue_.emplace(std::move(pending_reponse));
                    }
                    // 通知主线程处理响应
                    uint64_t value = 1;
                    eventfd_write(event_fd_, value);
                });
                //测试用：单线程处理请求
                /*parseRequest(client_fd, request, data);
                HttpResponse response = HandleRequest(request);
                PendingResponse pending_reponse{client_fd, response.GetResponse()};
                SendResponse(pending_reponse.clinet_fd, pending_reponse.data);*/
            }else{
                // 请求不完整
                LOG_INFO("请求不完整");
                ColseConnection(client_fd);
                return;
            }
        }else if(n == 0){
            //对端关闭连接
            LOG_INFO("客户端关闭连接");
            //关闭连接
            ColseConnection(client_fd);
            return;
        }else {
            //读取出错
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                //数据读完了，等待下一次事件
                return;
            }else {
                LOG_ERROR("recv错误:%s",strerror(errno));
                ColseConnection(client_fd);
                return;
            }

        }
    }
}     

// 返回HTTP响应
void WebServer::SendResponse(PendingResponse& to_response){
    // 发送响应
    size_t send_len = 0;
    size_t to_send = to_response.data.size();
    while(to_send > send_len){
        ssize_t len = send(to_response.clinet_fd, to_response.data.c_str() + send_len, to_send - send_len, 0);
        if(len < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                return;
            }else {
                LOG_ERROR("send错误:%s", strerror(errno));
                return;
            }
        }
        send_len += len;
    }

}

// 解析HTTP请求
void WebServer::parseRequest(int client_fd, HttpRequest& request, const std::string& data){
    if(request.Parse(data)){
        // 处理HTTP请求            
        }else{
            // 解析请求失败，关闭连接
            LOG_ERROR("解析请求失败，关闭连接");
            ColseConnection(client_fd);
        }
}

// 返回响应
HttpResponse WebServer::HandleRequest(const HttpRequest& request){
    //模拟数据库查询10毫秒
    //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    HttpResponse response;
    response.SetStatusCode(200);
    response.SetStatusMessage("OK");
    response.AddHeader("Content-Type", "text/html");
    response.SetBody("<html><body><h1>Hello, World!</h1></body></html>");
    return response;
}                   

//关闭连接
void WebServer::ColseConnection(int client_fd){
    //删除定时器
    timer_.DelTimer(client_fd);
    //删除epoll事件
    epoll_.Delete(client_fd);
    //关闭套接字
    close(client_fd);
    //删除缓冲区
    client_buffer_.erase(client_fd);
}