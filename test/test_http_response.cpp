#include "http/HttpResponse.h"
#include <iostream>

int main() {
    std::cout << "=== HttpResponse 测试程序 ===" << std::endl;
    //测试1：200 OK
    HttpResponse response;
    response.AddHeader("Content-Type", "text/html; charset=utf-8");
    response.SetBody("<html><body><h1>Hello World!</h1></body></html>");
    std::cout << response.GetResponse() << std::endl;
    //测试2：404 Not Found
    response.SetStatusCode(404);
    response.SetStatusMessage("Not Found");
    response.SetBody("<html><body><h1>404 Not Found</h1></body></html>");
    std::cout << response.GetResponse() << std::endl;
    //测试3：JSON
    std::cout << "\n=== JSON 测试 ===" << std::endl;
    HttpResponse response3;
    response3.AddHeader("Content-Type", "application/json");
    response3.SetBody("{\"code\": 0, \"message\": \"success\"}");
    std::cout << response3.GetResponse() << std::endl;

    std::cout << "\n===测试完成===" << std::endl;
    return 0;
}