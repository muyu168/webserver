#include "http/HttpRequestFSM.h"
#include <cstddef>
#include <iostream>

//解析请求行
LineState HttpRequestFSM::ParseRequestLine(std::string& data){
    size_t first_space= data.find(' ');
    if(first_space == std::string::npos){
        return LineState::LINE_BAD;
    }
    m_method_ = data.substr(0,first_space);
    size_t second_space = data.find(' ',first_space + 1);
    if(second_space == std::string::npos){
        return LineState::LINE_BAD;
    }
    m_url_ = data.substr(first_space + 1, second_space - first_space - 1);
    m_version_ = data.substr(second_space + 1);
    return LineState::LINE_OK;
}
//解析请求头
LineState HttpRequestFSM::ParseRequestHeaders(std::string& data){
    size_t colon = data.find(':');
    if(colon == std::string::npos){
        return LineState::LINE_BAD;
    }
    std::string key = data.substr(0, colon);
    //跳过冒号和空格
    size_t value_start = colon + 1;
    while(value_start < data.size() && data[value_start] == ' '){
        value_start++;
    }
    std::string value = data.substr(value_start);
    m_headers_[key] = value;
    return LineState::LINE_OK;
}
//解析请求体
LineState HttpRequestFSM::ParseRequestBody(std::string& data){
    if(data.size() >= m_content_lenth_){
        //如果剩余数据大于等于请求体长度，则直接取出请求体
        m_body_ += data.substr(0, m_content_lenth_);
        data.erase(0, m_content_lenth_);
    }else {
        //如果剩余数据小于请求体长度，则等待下次解析
        return LineState::LINE_MORE;
    }
    return LineState::LINE_OK;
}
//增量解析，返回当前状态
ParseState HttpRequestFSM::Parse(std::string& data){
    //解析状态机
    size_t start = 0;
    while(start < data.size() && m_line_state_ == LineState::LINE_OK){
        switch(m_parse_state_){
            case ParseState::REQUEST_LINE:{
                //找到请求行
                size_t end = data.find("\r\n");
                if(end == std::string::npos){
                    m_line_state_ = LineState::LINE_MORE;
                    break;
                }
                std::string request_line = data.substr(start, end);
                auto ret = ParseRequestLine(request_line);
                if(ret == LineState::LINE_BAD){
                    //请求行错误
                    m_parse_state_ = ParseState::ERROR;
                    m_line_state_ = ret;
                }else {
                    //请求行解析成功
                    m_line_state_ = ret;
                }
                data.erase(start, end + 2);
                m_parse_state_ = ParseState::REQUEST_HEADERS;
                break;
            }
            case ParseState::REQUEST_HEADERS:{
                //找到请求头
                size_t end = data.find("\r\n");
                if(end == std::string::npos){
                    m_line_state_ = LineState::LINE_MORE;
                    break;
                }
                if(end == 0){
                    //请求头结束
                    data.erase(start, 2);
                    //判断是否有请求体
                    auto it = m_headers_.find("Content-Length");
                    if(it != m_headers_.end() && std::stoi(it->second) > 0){
                        m_content_lenth_ = std::stoi(it->second);
                        m_parse_state_ = ParseState::REQUEST_BODY;
                    }else {
                        m_parse_state_ = ParseState::FINISH;
                    }
                    break;
                }
                //解析一行请求头
                std::string headers = data.substr(start, end); 
                auto ret = ParseRequestHeaders(headers);
                if(ret == LineState::LINE_BAD){
                    //请求头错误
                    m_parse_state_ = ParseState::ERROR;
                    m_line_state_ = ret;
                }else {
                    //请求头解析成功或需要更多数据
                    m_line_state_ = ret;
                }
                data.erase(start, end + 2);
                break;
            }
            case ParseState::REQUEST_BODY:{
                //获取请求体
                auto ret = ParseRequestBody(data);
                m_line_state_ = ret;
                if(m_line_state_ == LineState::LINE_OK){
                    //请求体解析完成
                    m_parse_state_ = ParseState::FINISH;
                }
                break;
            }
            case ParseState::FINISH:
                //解析完成
                return m_parse_state_;
            }

    }
    return m_parse_state_;
}

 //重置状态，用于长连接
void HttpRequestFSM::Reset(){
    m_line_state_ = LineState::LINE_OK;
    m_parse_state_ = ParseState::REQUEST_LINE;
    m_method_.clear();
    m_url_.clear();
    m_version_.clear();
    m_headers_.clear();
    m_body_.clear();
    m_content_lenth_ = 0;
}

//判断是否需要长连接
bool HttpRequestFSM::IsKeepAlive() const {
    auto it = m_headers_.find("Connection");
    if(it != m_headers_.end()){
        if(it->second != "close"){
            return true;
        }else {
            return false;
        }
    }else {
        //HTTP/1.1 默认 keep-alive，HTTP/1.0 默认 close
        return (m_version_ == "HTTP/1.1");
    }
}