#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <atomic>
#include <string.h>
#include <cstdarg>

enum class LogLevel {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR,
    FATAL
};

//固定大小的缓冲区
class LogBuffer {
public:
    static const int kBufferSize = 4 * 1024 * 1024;         //4MB

    LogBuffer() : cur_(0){};
    
    void Append(const char* data, size_t len);
    const char* Data() const { return data_; };             //获取数据
    size_t Length() const { return cur_; };                 //获取长度
    size_t Avail() const { return kBufferSize - cur_; };    //获取获取剩余长度
    void Clear() { cur_ = 0; };                             //清空缓冲区

private:
    char data_[kBufferSize];
    size_t cur_;    //当前写入的位置  
};

//日志系统
class Logger {
public:
    //获取单例（C++线程安全）
    static Logger& Instance();

    //初始化
    void Init(const std::string& log_path_ = "./logs/webserver.log",
        LogLevel log_level_ = LogLevel::INFO,
        int flush_interval_ = 3); //默认3秒刷新一次

    //核心日志方法
    void Log(LogLevel level, const char* file, int line, const char* fmt,...);

    //启动/停止后台进程
    void Start();
    void Stop();

    //设置日志级别（过滤低于此级别的日志）
    void SetLogLevel(LogLevel level) {log_level_ = level;};

    //禁止拷贝
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

private:
    Logger();                                                   //私有构造函数（单例模式）
    ~Logger();

    //后台线程函数
    void BackgroundThread();  

    //格式化日志
    void FormatLog(LogBuffer buffer,LogLevel level,const char* file, int line, const char* fmt, va_list args);

    //获取当前时间字符串
    std::string GetCurrentTime();

    //日志级别转字符串
    const char* LevelToString(LogLevel level);

    
private:
    //成员变量
    
    //双缓冲
    std::unique_ptr<LogBuffer> current_buffer_;
    std::unique_ptr<LogBuffer> next_buffer_;
    std::vector<std::unique_ptr<LogBuffer>> buffers_to_write_;     //待写入的缓冲区

    //线程同步
    std::mutex mutex_;
    std::condition_variable cond_;
    std::thread background_thread_;
    bool running_;

    //日志文件
    std::ofstream log_file_;                                        //日志文件
    std::string log_path_;                                          //日志文件路径
    LogLevel log_level_;                                            //当前日志级别
    int flush_interval_;                                            //刷新间隔

};

#define LOG_DEBUG(fmt, ...) \
    Logger::Instance().Log(LogLevel::DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_INFO(fmt, ...) \
    Logger::Instance().Log(LogLevel::INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    Logger::Instance().Log(LogLevel::WARN, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    Logger::Instance().Log(LogLevel::ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#define LOG_FATAL(fmt, ...) \
    Logger::Instance().Log(LogLevel::FATAL, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#endif