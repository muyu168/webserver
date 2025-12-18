#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>

class HttpRequest {
public:
    HttpRequest(){};
    ~HttpRequest(){};

    //解析Http请求
    bool Parse(const std::string& raw_request);

    //判断请求是否完整
    static bool IsComplete(const std::string& buffer);

    //获取请求信息
    std::string GetMethod() const { return method_;};
    std::string GetPath() const {return path_;};
    std::string GetVersion() const {return version_;};
    std::string GetHeader(const std::string& key) const;
    std::string GetBody() const {return body_;};

    //调试用：打印请求信息
    void Print() const;

private:
    //解析请求行
    bool ParseRequestLine(const std::string& line);

    //解析请求头
    void ParseHeaders(const std::string& header_part);



private:
    std::string method_;                            // GET, POST, PUT, DELETE, HEAD, etc.
    std::string path_;                              // /index.html, /login.php, etc.
    std::string version_;                           // HTTP/1.1, HTTP/2.0, etc.
    std::map<std::string,std::string> headers_;     // key-value pairs of headers
    std::string body_;                              // request body

};




#endif