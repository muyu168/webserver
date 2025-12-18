#include "http/HttpResponse.h"
#include <iostream>
#include <sstream>

    //获取完整的HTTP响应字符串
std::string HttpResponse::GetResponse() const{
    //构造HTTP响应头
    std::stringstream ss;
    ss << "HTTP/1.1" << " " << status_code_ << " " << status_msg_ << "\r\n";
    //如果有响应体，则添加Content-Length头
    if(!body_.empty()){
            ss << "Content-Length: " << body_.size() << "\r\n";
    }
    //添加所有响应头
    for(auto header : headers_){
        ss << header.first << ": " << header.second << "\r\n";
    }
    //添加空行
    ss << "\r\n";
    //添加响应体
    if(!body_.empty()){
        ss << body_;
    }
    //返回完整的HTTP响应字符串
    return ss.str();
}

    //调试用：打印
void HttpResponse::Print() const{
    std::cout << "HTTP Response:\n" << GetResponse() << std::endl;
}