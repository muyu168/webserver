#include "pool/ThreadPool.h"
#include "base/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== 线程池+日志测试开始 ===" << std::endl;
    
    // 初始化日志系统
    Logger::Instance().Init("./logs/threadpool.log", LogLevel::DEBUG);
    LOG_INFO("=== 测试程序启动 ===");
    
    {
        LOG_INFO("创建了线程池");
        ThreadPool pool(4);
        
        // 提交一些任务
        for (int i = 0; i < 10; ++i) {
            pool.Enqueue([i] {
                LOG_INFO("任务 %d 开始", i);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                LOG_INFO("任务 %d 结束", i);
            });
        }
        
        LOG_INFO("所有任务添加完成，等待工作进行");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        LOG_INFO("线程池即将关闭");
    }
    
    LOG_INFO("=== 测试程序结束 ===");
    Logger::Instance().Stop();
    
    std::cout << "=== 测试完成，查看 logs/threadpool.log ===" << std::endl;
    
    return 0;
}
