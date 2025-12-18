#include "http/HttpRequest.h"
#include <iostream>
#include <sstream>

    //解析Http请求
bool HttpRequest::Parse(const std::string& raw_request){
    if(!IsComplete(raw_request)){
        return false;
    }
    //找到请求行
    size_t pos = raw_request.find("\r\n");
    std::string request_line = raw_request.substr(0,pos);
    
    //解析请求行
    if(!ParseRequestLine(request_line)){
        return false;
    }

    //找到请求头
    size_t header_end = raw_request.find("\r\n\r\n");

    //解析请求头
    std::string header_part = raw_request.substr(pos + 2,header_end - pos - 2);
    ParseHeaders(header_part);

    //提取请求体
    body_ = raw_request.substr(header_end + 4);

    return true;
}

    //判断请求是否完整
bool HttpRequest::IsComplete(const std::string& buffer){
    if(buffer.find("\r\n\r\n") == std::string::npos){
        return false;
    }
    return true;
}

    //解析请求行
bool HttpRequest::ParseRequestLine(const std::string& line){
    size_t first_space = line.find(' ');
    if(first_space == std::string::npos){
        return false;
    }
    method_ = line.substr(0,first_space);
    size_t second_space = line.find(' ',first_space + 1);
    if(second_space == std::string::npos){
        return false;
    }
    path_ = line.substr(first_space + 1,second_space - first_space - 1);
    version_ = line.substr(second_space + 1);
    return true;
}

    //解析请求头
void HttpRequest::ParseHeaders(const std::string& header_part){
    size_t start = 0;

    while(start < header_part.size()){
        size_t line_end = header_part.find("\r\n",start);

        std::string line = header_part.substr(start,line_end - start);
         if(line_end == std::string::npos){
            break;  // 找不到更多行了，退出循环
        }

        size_t colon = line.find(": ");
        if(colon == std::string::npos){
            //忽略无效的请求头
            start = line_end + 2;
            continue;
        }
        std::string key = line.substr(0,colon);
        std::string value = line.substr(colon + 2);
        headers_[key] = value;
        start = line_end + 2;
    }
}
    //获取请求头
std::string HttpRequest::GetHeader(const std::string& key) const{
    auto it = headers_.find(key);
    if(it != headers_.end()){
        return it->second;
    }
    return "";
}


    //调试用：打印请求信息
void HttpRequest::Print() const{
    std::cout << "===HTTP Request===" << std::endl;
    std::cout << "Method: " << method_ << std::endl;
    std::cout << "Path: " << path_ << std::endl;
    std::cout << "Version: " << version_ << std::endl;
    std::cout << "Headers: " << std::endl;
    for(const auto& pair : headers_){
        std::cout << pair.first << ": " << pair.second << std::endl;
    }
    if(!body_.empty()){
        std::cout << "Body: " << body_ <<std::endl;
    }
}
