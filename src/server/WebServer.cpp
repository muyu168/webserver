#include "server/WebServer.h"
#include "base/Logger.h"
#include "http/HttpRequestFSM.h"
#include "http/MimeType.h"
#include <chrono>
#include <csignal>
#include <iostream>
#include <arpa/inet.h>
#include <iterator>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/signalfd.h>

#define INET_ADDRSTRLEN 16
#define TIMEOUT_MS 60000
// 启动服务器 
bool WebServer::Start(){
    LOG_INFO("WebServer 启动");
    signal(SIGPIPE, SIG_IGN);
    server_socket_.Create();
    server_socket_.SetNonBlocking();                                                // 设置非阻塞模式                                                         // 创建服务器套接字
    server_socket_.SetReuseAddr();                                                  // 设置 SO_REUSEADDR 选项
    server_socket_.Bind("",port_);                                                      // 绑定端口
    epoll_.Add(server_socket_.GetFd(), EPOLLIN | EPOLLET);                          // 注册服务器套接字到 epoll
    
    event_fd_ = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC); 
    epoll_.Add(event_fd_, EPOLLIN | EPOLLET); 

    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    
    signal_fd_ = signalfd(-1, &sigset, SFD_NONBLOCK | SFD_CLOEXEC);
    if(signal_fd_ < 0){
        LOG_ERROR("signalfd创建失败");
        return false;
    }
    LOG_INFO("signalfd 创建成功, fd=%d", signal_fd_); 
    if(!epoll_.Add(signal_fd_, EPOLLIN | EPOLLET)){
    LOG_ERROR("signalfd 加入 epoll 失败: %s", strerror(errno));
    return false;
    }
    LOG_INFO("signalfd 已加入 epoll");
    

    running_ = true;                                                                // 服务器运行状态
    server_socket_.Listen(128);                                                     // 监听端口
    LOG_INFO("WebServer 启动成功");
    Run();
    Stop();
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
            LOG_DEBUG("收到事件, fd=%d", client_fd);
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
            }else if(client_fd == signal_fd_){
                // 处理信号事件
                LOG_INFO("检测到 signal_fd 事件");
                HandleSignal(client_fd);
            }else {
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
    LOG_INFO("开始清理资源");
    //关闭所有连接
    for(auto& client_fd : client_buffer_){
        CloseConnection(client_fd.first);
    }
    //清理缓存区
    client_buffer_.clear();
    //清理请求状态机
    request_fsm_.clear();
    //关闭服务器套接字
    epoll_.Delete(server_socket_.GetFd());
    server_socket_.Close();
    //关闭事件通知
    epoll_.Delete(event_fd_);
    close(event_fd_);
    //关闭信号通知
    epoll_.Delete(signal_fd_);
    close(signal_fd_);
    LOG_INFO("资源清理完毕,WebServer 停止");
}

// 处理新连接请求   
void WebServer::HandleNewConnection(){
    while(true){
        struct sockaddr_in client_addr;
        int client_fd_ = server_socket_.Accept(&client_addr);
        if(client_fd_ > 0){
            SetNoBlocking(client_fd_);
            epoll_.Add(client_fd_, EPOLLIN | EPOLLET | EPOLLONESHOT);
            //为新连接添加定时器
            auto ms = std::chrono::milliseconds(TIMEOUT_MS);
            timer_.AddTimer(client_fd_, ms, [this,client_fd = client_fd_](){
                CloseConnection(client_fd);
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
            // 【线程安全】加锁保护 client_buffer_
            {
                std::unique_lock<std::mutex> lock(buffer_mutex_);
                client_buffer_[client_fd] += std::string(buf, n);
                if(client_buffer_[client_fd].size() > 65536){  // 提高限制到 64KB
                    // 请求过大，关闭连接
                    LOG_ERROR("fd=%d 请求过大(%zu bytes)，关闭连接", client_fd, client_buffer_[client_fd].size());
                    CloseConnection(client_fd);
                    return;
                }
            }

            // 收到客户端事件，调整定时器
            auto ms = std::chrono::milliseconds(TIMEOUT_MS);
            timer_.AdjustTimer(client_fd, ms, [this, fd = client_fd](){
                CloseConnection(fd);
            });

        }else if(n == 0){
            // 对端关闭连接
            LOG_INFO("fd=%d 客户端关闭连接", client_fd);
            CloseConnection(client_fd);
            return;
        }else {
            // 读取出错
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 数据读完了，提交到线程池处理

                // 【线程安全】从缓冲区取出数据并清空
                std::string data;
                {
                    std::unique_lock<std::mutex> lock(buffer_mutex_);
                    data = std::move(client_buffer_[client_fd]);
                    client_buffer_[client_fd].clear();  // 确保清空
                }

                // 提交到线程池异步处理
                thread_pool_.Enqueue([this, client_fd, data = std::move(data)]() mutable {
                    // 【线程安全】获取或创建请求状态机
                    HttpRequestFSM request;
                    {
                        std::unique_lock<std::mutex> lock(fsm_mutex_);
                        if(request_fsm_.find(client_fd) != request_fsm_.end()){
                            request = request_fsm_[client_fd];
                        }
                    }

                    // 解析请求（可能有多个请求在一个数据包中）
                    while(!data.empty()){
                        request.Parse(data);

                        if(request.IsComplete()){
                            // 请求完整，处理请求
                            HttpResponse response = HandleRequest(request);
                            PendingResponse pending_response{
                                client_fd,
                                response.GetResponse(),
                                response.ShouldKeepAlive()  // 保存 Keep-Alive 状态
                            };

                            // 保存响应到队列
                            {
                                std::unique_lock<std::mutex> lock(response_mutex_);
                                response_queue_.emplace(std::move(pending_response));
                            }

                            // 重置状态机，准备处理下一个请求
                            request.Reset();

                            // 通知主线程处理响应
                            uint64_t value = 1;
                            eventfd_write(event_fd_, value);

                        }else if(request.IsError()){
                            // 请求出错，关闭连接
                            LOG_ERROR("fd=%d 请求解析出错，关闭连接", client_fd);
                            CloseConnection(client_fd);
                            return;

                        }else{
                            // 请求不完整，保存剩余数据和状态机
                            {
                                std::unique_lock<std::mutex> lock(buffer_mutex_);
                                client_buffer_[client_fd] += data;
                            }
                            {
                                std::unique_lock<std::mutex> lock(fsm_mutex_);
                                request_fsm_[client_fd] = request;
                            }
                            break;  // 等待更多数据
                        }
                    }

                    // 处理完成后，重新注册 epoll 事件（EPOLLONESHOT 需要重新激活）
                    if(!epoll_.ReArm(client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT)){
                        LOG_ERROR("fd=%d ReArm 失败，  关闭连接", client_fd);
                        CloseConnection(client_fd);
                    }
                });
                return;  
            }else {
                LOG_ERROR("fd=%d recv错误: %s", client_fd, strerror(errno));
                CloseConnection(client_fd);
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
        ssize_t len = send(to_response.client_fd, to_response.data.c_str() + send_len, to_send - send_len, 0);
        if(len < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 发送缓冲区满，稍后重试
                LOG_DEBUG("fd=%d 发送缓冲区满，已发送 %zu/%zu bytes", to_response.client_fd, send_len, to_send);
                return;
            }else {
                LOG_ERROR("fd=%d send错误: %s", to_response.client_fd, strerror(errno));
                CloseConnection(to_response.client_fd);
                return;
            }
        }
        send_len += len;
    }

    LOG_DEBUG("fd=%d 响应发送完成，共 %zu bytes", to_response.client_fd, send_len);

    // 【Keep-Alive 支持】根据响应头决定是否关闭连接
    if(!to_response.keep_alive){
        // 短连接模式，发送完响应后关闭连接
        LOG_DEBUG("fd=%d 短连接模式，关闭连接", to_response.client_fd);
        CloseConnection(to_response.client_fd);
    }else{
        // 长连接模式，重新注册 epoll 事件，等待下一个请求
        LOG_DEBUG("fd=%d 长连接模式，保持连接", to_response.client_fd);
        if(!epoll_.ReArm(to_response.client_fd, EPOLLIN | EPOLLET | EPOLLONESHOT)){
            LOG_ERROR("fd=%d ReArm 失败，关闭连接", to_response.client_fd);
            CloseConnection(to_response.client_fd);
        }
    }
}

// 解析HTTP请求
void WebServer::parseRequest(int client_fd, HttpRequest& request, const std::string& data){
    if(request.Parse(data)){
        // 处理HTTP请求            
        }else{
            // 解析请求失败，关闭连接
            LOG_ERROR("解析请求失败，关闭连接");
            CloseConnection(client_fd);
        }
}

// 返回响应
HttpResponse WebServer::HandleRequest(const HttpRequestFSM& request){
    HttpResponse response;

    // 获取请求方法和 URL
    std::string method = request.GetMethod();
    std::string url = request.GetUrl();

    LOG_DEBUG("处理请求: %s %s", method.c_str(), url.c_str());

    // 判断 URL 是否合法（防止目录遍历攻击）
    if(url.find("..") != std::string::npos){
        // URL 不合法，返回 403
        response.SetStatusCode(403);
        response.SetStatusMessage("Forbidden");
        response.SetBody("<html><body><h1>403 Forbidden</h1><p>Access denied.</p></body></html>");
        response.AddHeader("Content-Type", "text/html");
        return response;
    }

    // 【POST 请求处理】
    if(method == "POST"){
        return HandlePostRequest(request);
    }

    // 【GET 请求处理】
    if(method != "GET" && method != "HEAD"){
        // 不支持的方法，返回 405
        response.SetStatusCode(405);
        response.SetStatusMessage("Method Not Allowed");
        response.SetBody("<html><body><h1>405 Method Not Allowed</h1><p>Only GET, HEAD, and POST are supported.</p></body></html>");
        response.AddHeader("Content-Type", "text/html");
        response.AddHeader("Allow", "GET, HEAD, POST");
        return response;
    }

    if(url == "/"){
        // 处理根目录请求
        url = "/index.html";
    }

    // 构建完整文件路径
    std::string file_path = root_path_ + url;

    // 检查文件是否存在
    struct stat file_stat;
    if(stat(file_path.c_str(), &file_stat) != 0){
        // 文件不存在，返回 404
        response.SetStatusCode(404);
        response.SetStatusMessage("Not Found");
        response.SetBody("<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>");
        response.AddHeader("Content-Type", "text/html");
        return response;
    }

    // HEAD 请求只返回头部，不返回 Body
    if(method == "HEAD"){
        response.SetStatusCode(200);
        response.SetStatusMessage("OK");
        response.AddHeader("Content-Type", MimeType::GetMimeType(file_path));
        response.AddHeader("Content-Length", std::to_string(file_stat.st_size));

        // Keep-Alive 支持
        if(request.IsKeepAlive()){
            response.AddHeader("Connection", "keep-alive");
            response.AddHeader("Keep-Alive", "timeout=60, max=100");
        }else{
            response.AddHeader("Connection", "close");
        }
        return response;
    }

    // 读取文件
    std::ifstream file(file_path, std::ios::binary);
    if(!file.is_open()){
        // 文件无法打开，返回 500
        response.SetStatusCode(500);
        response.SetStatusMessage("Internal Server Error");
        response.SetBody("<html><body><h1>500 Internal Server Error</h1><p>Failed to open file.</p></body></html>");
        response.AddHeader("Content-Type", "text/html");
        return response;
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // 构建成功响应
    response.SetStatusCode(200);
    response.SetStatusMessage("OK");
    response.AddHeader("Content-Type", MimeType::GetMimeType(file_path));

    // 【Keep-Alive 支持】根据请求决定是否保持连接
    if(request.IsKeepAlive()){
        response.AddHeader("Connection", "keep-alive");
        response.AddHeader("Keep-Alive", "timeout=60, max=100");  // 60秒超时，最多100个请求
    }else{
        response.AddHeader("Connection", "close");
    }

    response.SetBody(content);
    return response;
}

// 处理 POST 请求
HttpResponse WebServer::HandlePostRequest(const HttpRequestFSM& request){
    HttpResponse response;
    std::string url = request.GetUrl();

    LOG_DEBUG("处理 POST 请求: %s, Body 长度: %zu", url.c_str(), request.GetBody().size());

    // 【示例1：表单提交接口】
    if(url == "/api/submit"){
        // 解析表单数据（application/x-www-form-urlencoded）
        std::string body = request.GetBody();

        // 简单的表单解析示例
        std::map<std::string, std::string> form_data;
        size_t pos = 0;
        while(pos < body.size()){
            size_t eq = body.find('=', pos);
            size_t amp = body.find('&', pos);
            if(eq == std::string::npos) break;

            std::string key = body.substr(pos, eq - pos);
            std::string value;
            if(amp == std::string::npos){
                value = body.substr(eq + 1);
                pos = body.size();
            }else{
                value = body.substr(eq + 1, amp - eq - 1);
                pos = amp + 1;
            }
            form_data[key] = value;
        }

        // 构建 JSON 响应
        std::string json_response = "{\"status\":\"success\",\"message\":\"Data received\",\"data\":{";
        bool first = true;
        for(const auto& pair : form_data){
            if(!first) json_response += ",";
            json_response += "\"" + pair.first + "\":\"" + pair.second + "\"";
            first = false;
        }
        json_response += "}}";

        response.SetStatusCode(200);
        response.SetStatusMessage("OK");
        response.AddHeader("Content-Type", "application/json");
        response.SetBody(json_response);

        LOG_INFO("POST /api/submit 处理成功，返回 %zu bytes", json_response.size());
    }
    // 【示例2：JSON 接口】
    else if(url == "/api/echo"){
        // 回显接收到的数据
        std::string body = request.GetBody();

        response.SetStatusCode(200);
        response.SetStatusMessage("OK");

        // 检查 Content-Type
        auto headers = request.GetHeaders();
        auto ct = headers.find("Content-Type");
        if(ct != headers.end() && ct->second.find("application/json") != std::string::npos){
            response.AddHeader("Content-Type", "application/json");
        }else{
            response.AddHeader("Content-Type", "text/plain");
        }

        response.SetBody(body);
        LOG_INFO("POST /api/echo 处理成功，回显 %zu bytes", body.size());
    }
    // 【默认：不支持的 POST 路径】
    else{
        response.SetStatusCode(404);
        response.SetStatusMessage("Not Found");
        response.SetBody("{\"status\":\"error\",\"message\":\"API endpoint not found\"}");
        response.AddHeader("Content-Type", "application/json");
        LOG_WARN("POST %s 未找到对应的处理接口", url.c_str());
    }

    // Keep-Alive 支持
    if(request.IsKeepAlive()){
        response.AddHeader("Connection", "keep-alive");
        response.AddHeader("Keep-Alive", "timeout=60, max=100");
    }else{
        response.AddHeader("Connection", "close");
    }

    return response;
}                   

//关闭连接
void WebServer::CloseConnection(int client_fd){
    LOG_INFO("关闭连接 fd=%d", client_fd);

    // 删除定时器
    timer_.DelTimer(client_fd);

    // 删除epoll事件
    epoll_.Delete(client_fd);

    // 关闭套接字
    close(client_fd);

    // 【线程安全】清理缓冲区和状态机
    {
        std::unique_lock<std::mutex> lock(buffer_mutex_);
        client_buffer_.erase(client_fd);
    }
    {
        std::unique_lock<std::mutex> lock(fsm_mutex_);
        request_fsm_.erase(client_fd);
    }
}

void WebServer::HandleSignal(int signal_fd){
    //读取信号数据
    struct signalfd_siginfo siginfo;
    ssize_t len = read(signal_fd, &siginfo, sizeof(siginfo));
    if(len != sizeof(siginfo)){
        LOG_ERROR("信号读取失败");
        return;
    }

    //处理信号
    if(siginfo.ssi_signo == SIGINT || siginfo.ssi_signo == SIGTERM){
        LOG_INFO("收到退出信号,准备关闭服务器");
        running_ = false;
        return;
    }
}