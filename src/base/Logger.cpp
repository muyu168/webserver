#include "base/Logger.h"
#include <iostream>

void LogBuffer::Append(const char* data, size_t len){
    if(len > Avail()){
        len = Avail();
    }
    memcpy(data_ + cur_, data, len); 
    cur_ += len;
}


Logger::Logger(): running_(false){
    
}
Logger::~Logger(){
    if(running_){
        Stop();
    }
}

 //初始化
void Logger::Init(const std::string& log_path ,LogLevel level,int flush_interval){ //默认3秒刷新一次
    log_path_ = log_path;       // 赋值给成员变量
    log_level_ = level;
    flush_interval_ = flush_interval;
    
    // 打开日志文件
    log_file_.open(log_path_, std::ios::app);
    if (!log_file_.is_open()) {
        std::cerr << "Failed to open log file: " << log_path_ << std::endl;
    }
    current_buffer_.reset(new LogBuffer);
    next_buffer_.reset(new LogBuffer);
    if(running_) return;
    running_ = true;
    background_thread_ = std::thread(&Logger::BackgroundThread, this);
}


//获取单例（C++线程安全）
Logger& Logger::Instance(){
    static Logger instance_;
    return instance_;
}


//核心日志方法
void Logger::Log(LogLevel level, const char* file, int line, const char* fmt,...){
    //过滤低级别日志
    if(level < log_level_) return;
    //格式化日志
    char temp[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);
    //构建完整日志行
    char log_line[1024];
    std::string time_str = GetCurrentTime();
    const char* level_str = LevelToString(level);
    //提取文件名
    const char* filename = strrchr(file, '/');
    filename = filename ? filename + 1 : file;
    int len = snprintf(log_line, sizeof(log_line), "[%s][%s][%s:%d] %s\n", time_str.c_str(), level_str, filename, line, temp);

    //加锁
    std::lock_guard<std::mutex> lock(mutex_);
    //写入
    if(current_buffer_->Avail() > len){
        //缓冲区还有位置就写入当前缓冲区
        current_buffer_->Append(log_line , len);
    }else {
        //缓冲区满了,切换
        buffers_to_write_.push_back(std::move(current_buffer_));
        if(next_buffer_){
            current_buffer_ = std::move(next_buffer_);
        }else {
            current_buffer_.reset(new LogBuffer);
        }
        current_buffer_->Append(log_line,len);
        cond_.notify_one();     //通知后台线程
    }
}

//启动/停止后台进程
void Logger::Start(){
    BackgroundThread();
}
void Logger::Stop(){
    {
    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    cond_.notify_one();//唤醒后台线程
    }
    if(background_thread_.joinable()){
        background_thread_.join();//等待进程退出
    }
    log_file_.close();
}


//后台线程函数
void Logger::BackgroundThread(){
    std::unique_ptr<LogBuffer> new_buffer1(new LogBuffer);
    std::unique_ptr<LogBuffer> new_buffer2(new LogBuffer);
    std::vector<std::unique_ptr<LogBuffer>> buffers;
    while(running_){
        std::unique_lock<std::mutex> lock(mutex_);
        if(buffers_to_write_.empty()){
            cond_.wait_for(lock, std::chrono::seconds(flush_interval_));
        }
        buffers_to_write_.push_back(std::move(current_buffer_));
        current_buffer_ = std::move(new_buffer1);
        buffers.swap(buffers_to_write_);
        if(!next_buffer_){
            next_buffer_ = std::move(new_buffer2);
        }

        //批量写入文件（无锁）
        for(auto& buffer : buffers){
            log_file_.write(buffer->Data(),buffer->Length());
        }
        log_file_.flush();

        //回收缓冲区
        buffers.clear();
        new_buffer1.reset(new LogBuffer);
        new_buffer2.reset(new LogBuffer);
    }
    //最后一次刷盘
    log_file_.flush();
}

//格式化日志
void Logger::FormatLog(LogBuffer buffer,LogLevel level,const char* file, int line, const char* fmt, va_list args){
    
}

//获取当前时间字符串
std::string Logger::GetCurrentTime(){
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    char buf[64];
    snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%03d",tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec, static_cast<int>(ms.count()));
    return std::string(buf);
}

//日志级别转字符串
const char* Logger::LevelToString(LogLevel level){
    switch(level){
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOW";
    }
}



