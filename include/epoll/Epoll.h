#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <vector>

class Epoll {
public:
    Epoll(int max_events = 1024);
    ~Epoll();

    //禁止拷贝
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;

    //添加文件描述符到epoll句柄
    bool Add(int fd,uint32_t events);

    //删除文件描述符
    bool Delete(int fd);

    //修改文件描述符的事件
    bool Modify(int fd,uint32_t events);

    //等待事件发生(返回就绪事件的数量)
    int Wait(int timeout = -1);

    //获取就绪的文件描述符
    int GetEventFd(size_t i) const {return events_[i].data.fd;};

    //获取就绪事件的类型
    uint32_t GetEvents(size_t i) const {return events_[i].events;};

private:
    int epoll_fd_;
    std::vector<struct epoll_event> events_;

};


#endif