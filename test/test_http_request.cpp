#include "http/HttpRequest.h"
#include <iostream>

int main() {
    std::cout << "=== HttpRequest 测试程序 ===" << std::endl;
    
    // 模拟一个 HTTP GET 请求
    std::string http_get_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    std::cout << "\n--- 测试 GET 请求 ---" << std::endl;
    std::cout << "原始请求:\n" << http_get_request << std::endl;
    
    HttpRequest request1;
    if (request1.Parse(http_get_request)) {
        std::cout << "✓ 解析成功！" << std::endl;
        request1.Print();
    } else {
        std::cout << "✗ 解析失败！" << std::endl;
    }
    
    // 测试 POST 请求
    std::string http_post_request = 
        "POST /api/login HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 27\r\n"
        "\r\n"
        "{\"user\":\"admin\",\"pwd\":\"123\"}";
    
    std::cout << "\n--- 测试 POST 请求 ---" << std::endl;
    std::cout << "原始请求:\n" << http_post_request << std::endl;
    
    HttpRequest request2;
    if (request2.Parse(http_post_request)) {
        std::cout << "✓ 解析成功！" << std::endl;
        request2.Print();
    } else {
        std::cout << "✗ 解析失败！" << std::endl;
    }
    
    // 测试不完整的请求
    std::cout << "\n--- 测试不完整请求 ---" << std::endl;
    std::string incomplete_request = "GET /index.html HTTP/1.1\r\n";
    std::cout << "不完整请求: " << incomplete_request << std::endl;
    
    if (HttpRequest::IsComplete(incomplete_request)) {
        std::cout << "✗ 错误：应该判断为不完整" << std::endl;
    } else {
        std::cout << "✓ 正确：判断为不完整" << std::endl;
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
}