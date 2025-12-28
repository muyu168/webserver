#include "base/Logger.h"
#include <iostream>
#include <unistd.h>

int main(){
    std::cout << "===日志系统测试开始===" << std::endl;

    //初始化日志系统
    Logger::Instance().Init("./test_log.txt", LogLevel::DEBUG, 3);

    //测试不同级别的日志
    LOG_DEBUG("这是一条DEBUG日志，数字: %d" , 21);
    LOG_ERROR("这是一条ERROR日志，字符串：%s" , "ERROR");
    LOG_FATAL("这是一条FATAL日志, 浮点数：%f" , 3.1415926);
    LOG_INFO("这是一条INFO日志");
    LOG_WARN("这是一条WARN日志");

    //大量日志写入（压力测试）
    std::cout << "写入1000条日志" << std::endl;
    for(int i = 0 ; i< 1000 ; i++){
        LOG_INFO("日志序号：%d" , i);
    }

    //等待日志刷盘
    std::cout << "等待3秒后台线程写入文件..." << std::endl;
    sleep(3);

    //测试日志级别过滤
    std::cout << "测试日志级别过滤开始，设置日志级别为WARN， EDBUG和INFO应该被过滤" << std::endl;
    Logger::Instance().SetLogLevel(LogLevel::WARN);
    LOG_DEBUG("这是一条DEBUG日志，不该出现");
    LOG_ERROR("这是一条ERROR日志");
    LOG_FATAL("这是一条FATAL日志");
    LOG_INFO("这是一条INFO日志，不该出现");
    LOG_WARN("这是一条WARN日志");

    //停止日志系统
    std::cout << "日志系统停止" << std::endl;
    Logger::Instance().Stop();


    std::cout << "===日志系统测试结束,请检查test_log.txt文件===" << std::endl;
    return 0;
}