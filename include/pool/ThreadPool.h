#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "base/Logger.h"
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_num = 8);

    ~ThreadPool();
   
    //提交任务，返回futrue获取结果
    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

    //禁止拷贝
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
private:
    std::vector<std::thread> workers_;              //工作线程数组
    std::queue<std::function<void()>> tasks_;       //任务队列
    std::mutex mutex_;                              //互斥锁
    std::condition_variable cond_;                  //条件变量
    bool stop_;                                     //停止标志
};

inline ThreadPool::ThreadPool(size_t thread_num) : stop_(false){
        //线程初始化
        for(int i = 0 ; i < thread_num ; i++){
            //依次初始化线程数组成员
            workers_.emplace_back(std::thread([this,i](){
                LOG_DEBUG("线程%d启动",i);  
                //工作函数
                while(true){
                    std::function<void()> task;
                    {
                        //加锁保证线程安全
                        std::unique_lock<std::mutex> lock(mutex_);
                        //等待条件变量
                        cond_.wait(lock,[this,i]{
                            //仅当线程池未停止且任务队列为空时等待
                            LOG_DEBUG("线程%d等待任务",i);
                            return stop_ || !tasks_.empty();
                        });
                        //如果线程池停止并且任务队列为空，直接返回
                        if(stop_ && tasks_.empty()) {
                            LOG_DEBUG("线程%d退出",i);
                            return;
                        }
                        //获取任务
                        task = tasks_.front();
                        tasks_.pop();
                        LOG_DEBUG("线程%d获取了一个任务", i);
                    }
                    //执行任务工作,获取异常
                    try {
                        task();
                    } catch(const std::exception& e) {
                        //记录日志
                        LOG_ERROR("任务出错：%s",e.what());
                    }
                    //记录线程信息
                    LOG_DEBUG("线程%d完成了一个任务，任务列表中还剩下%d个任务", i, tasks_.size());
                }
            }));
        }
    }

inline ThreadPool::~ThreadPool(){
        {
            //加锁保证线程安全
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }  
        LOG_INFO("线程池正在停止，通知所有工作线程");
        //唤醒所有线程检查stop状态
        cond_.notify_all();
        //回收线程资源
        for(auto &worker : workers_){
            worker.join();
        }
        LOG_INFO("线程池已经停止，所有线程已退出");
    }

template<typename F, typename... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args)-> std::future<typename std::result_of<F(Args...)>::type>{
    using return_type = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<return_type>(args)...)
    );
    std::future<return_type> res = task->get_future();
        {
            //添加任务到任务队列
            std::unique_lock<std::mutex> lock(mutex_);
            if(stop_) {
                throw std::runtime_error("线程池已停止运行");
            }
            tasks_.emplace([task]() { (*task)(); });
            LOG_DEBUG("添加新任务，现有待处理任务数为%d", tasks_.size());
        }
        //通知工作线程工作
        cond_.notify_one();
        return res;
    }

#endif