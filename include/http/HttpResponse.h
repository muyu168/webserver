#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>
#include <map>

class HttpResponse {
public:
    HttpResponse() : status_code_(200) , status_msg_("OK"){};
    ~HttpResponse() = default;

    //设置状态码
    void SetStatusCode(int code) {status_code_ = code;};
    void SetStatusMessage(const std::string& msg) {status_msg_ = msg;};

    //添加响应头
    void AddHeader(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }

    //设置响应体
    void SetBody(const std::string& body){
        body_ = body;
    }

    //获取完整的HTTP响应字符串
    std::string GetResponse() const;

    //调试用：打印
    void Print() const;


private:
    int status_code_;                               //200,404,500, etc.
    std::string status_msg_;                        //OK,Not Found, etc.
    std::map<std::string,std::string> headers_;     //响应头
    std::string body_;                              //响应体
};

#endif