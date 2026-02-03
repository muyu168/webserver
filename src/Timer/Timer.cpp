#include "timer/Timer.h"
#include <chrono>
#include <iostream>


//添加定时器
void Timer::AddTimer(int fd, Milliseconds timeout_ms, std::function<void()> callback){
    TimerNode node;
    auto now = Clock::now();
    node.fd = fd;
    node.expire = now + timeout_ms;
    node.callback = callback;
    timer_queue_.push(node);
    timer_map_[fd] = node.expire;
}
//删除定时器
void Timer::DelTimer(int fd){
    timer_map_.erase(fd);
}
//调整超时时间
void Timer::AdjustTimer(int fd, Milliseconds timeout_ms, std::function<void()> callback){
    TimerNode node;
    auto now = Clock::now();
    node.fd = fd;
    node.expire = now + timeout_ms;
    node.callback = callback;
    timer_queue_.push(node);
    timer_map_[fd] = node.expire;
}
//返回距离最近超时的毫秒数
int Timer::GetNextTimeout(){
    if (timer_queue_.empty()) {
        return -1;
    }
    auto now = Clock::now();
    auto diff = timer_queue_.top().expire - now;
    auto ms = std::chrono::duration_cast<Milliseconds>(diff).count();
    return ms > 0 ? static_cast<int>(ms) : 0;
}

//检查并执行超时的定时器
void Timer::Tick(){
    auto now = Clock::now();
    while(!timer_queue_.empty() && timer_queue_.top().expire <= now ){
        auto it = timer_map_.find(timer_queue_.top().fd);
        //如果定时器已经被删除或已经过期，则跳过
        if(it == timer_map_.end() || timer_queue_.top().expire != it->second){
            timer_queue_.pop();
            continue;
        }      
        auto node = timer_queue_.top();
        node.callback();
        timer_queue_.pop();
        timer_map_.erase(node.fd);
    }

}