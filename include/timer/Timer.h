#ifndef TIMER_H
#define TIMER_H

#include <queue>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>

struct TimerNode {
    int fd;
    std::chrono::steady_clock::time_point expire;
    std::function<void()> callback;

    bool operator > (const TimerNode& t) const {
        return expire > t.expire;
    }
};


class Timer {
public:
    
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Milliseconds = std::chrono::milliseconds;

    Timer() = default;
    ~Timer() = default;

    //添加定时器
    void AddTimer(int fd, Milliseconds timeout_ms, std::function<void()> callback);
    //删除定时器
    void DelTimer(int fd);
    //调整超时时间
    void AdjustTimer(int fd, Milliseconds timeout_ms, std::function<void()> callback);
    //返回距离最近超时的毫秒数
    int GetNextTimeout();
    //检查并执行超时的定时器
    void Tick();

private:
    std::priority_queue<TimerNode, std::vector<TimerNode>, std::greater<TimerNode>> timer_queue_;
    std::unordered_map<int, TimePoint> timer_map_;
};

#endif