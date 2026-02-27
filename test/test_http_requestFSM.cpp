#include "http/HttpRequestFSM.h"
#include <iostream>

int main(){
    std::cout << "=== HttpRequestFSM测试开始 ===" << std::endl;
    HttpRequestFSM fsm;
    // 模拟一个 HTTP GET 请求
    std::string http_get_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    // 解析 HTTP GET 请求
    std::cout << "--- 测试 HTTP GET 请求 ---" << std::endl;
    std::cout << "原始请求:\n" << http_get_request << std::endl;
    fsm.Parse(http_get_request);
    std::cout << "请求方法" << fsm.GetMethod() << std::endl;
    std::cout << "请求头:" ;
    for(auto& header : fsm.GetHeaders()){
        std::cout << header.first << ":" << header.second << std::endl;
    }
    std::cout << "请求路径" << fsm.GetUrl() << std::endl;
    std::cout << "HTTP版本" << fsm.GetVersion() << std::endl;

    // 测试 POST 请求
    std::string http_post_request = 
        "POST /api/login HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"user\":\"admin\",\"pwd\":\"123\"}";
    
    // 解析 HTTP POST 请求
    std::cout << "\n--- 测试 HTTP POST 请求 ---" << std::endl;
    std::cout << "原始请求:\n" << http_post_request << std::endl;
    fsm.Parse(http_post_request);
    std::cout << "请求方法" << fsm.GetMethod() << std::endl;
    std::cout << "请求头:" ;
    for(auto& header : fsm.GetHeaders()){
        std::cout << header.first << ":" << header.second << std::endl;
    }
    std::cout << "请求路径" << fsm.GetUrl() << std::endl;
    std::cout << "HTTP版本" << fsm.GetVersion() << std::endl;
    std::cout << "请求体" << fsm.GetBody() << std::endl;

    // 测试不完整请求
    std::string incomplete_request = "GET /index.html HTTP/1.1\r\n";
    std::cout << "\n--- 测试不完整请求 ---" << std::endl;
    std::cout << "不完整请求: " << incomplete_request << std::endl;
    fsm.Parse(incomplete_request);
    if(fsm.IsComplete()){
        std::cout << "请求不完整" << std::endl;
    }

    std::cout << "\n=== 测试结束 ===" << std::endl;

    
    return 0;
}
