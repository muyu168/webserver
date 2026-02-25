#ifndef HTTPREQUESTFSM_H
#define HTTPREQUESTFSM_H

#include <string>
#include <map>

enum class ParseState{
    REQUEST_LINE,       //正在解析请求行
    REQUEST_HEADERS,    //正在解析头部
    REQUEST_BODY,       //正在解析请求体
    FINISH,             //解析完成
    ERROR               //解析出错
};

enum class LineState{
    LINE_OK,             //行解析成功
    LINE_BAD,            //行解析失败
    LINE_MORE            //需要更多数据
};


class HttpRequestFSM{
public:
    HttpRequestFSM(){};
    ~HttpRequestFSM(){};
 
    //增量解析，返回当前状态
    ParseState Parse(std::string& data);

    //重置状态，用于长连接
    void Reset();

    //判断是否需要长连接
    bool IsKeepAlive();

    //获取请求方法
    const std::string& GetMethod()const{return m_method_;};
    //获取请求路径
    const std::string& GetUrl()const{return m_url_;};
    //获取HTTP版本
    const std::string& GetVersion()const{return m_version_;};
    //获取请求头
    const std::map<std::string, std::string>& GetHeaders()const{return m_headers_;};
    //获取请求体
    const std::string& GetBody()const{return m_body_;};
    //判断请求是否完成
    bool IsComplete()const{return m_parse_state_ == ParseState::FINISH;};
private:
    //解析请求行
    LineState ParseRequestLine(std::string& data);
    //解析请求头
    LineState ParseRequestHeaders(std::string& data);
    //解析请求体
    LineState ParseRequestBody(std::string& data);

private:
    LineState m_line_state_ = LineState::LINE_OK;           //当前行解析状态
    ParseState m_parse_state_ = ParseState::REQUEST_LINE;   //当前状态
    std::string m_method_;                                  //请求方法  
    std::string m_url_;                                     //请求路径
    std::string m_version_;                                 //HTTP版本
    std::map<std::string, std::string> m_headers_;          //请求头
    std::string m_body_;                                    //请求体
    size_t m_content_lenth_;                                //请求体长度(用于判断body是否读完)
    bool m_keep_alive_ = false;                             //是否保持长连接
};



#endif