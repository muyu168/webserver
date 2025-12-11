#include "epoll/Epoll.h"
#include <iostream>
#include <unistd.h>

Epoll::Epoll(int max_events) {
    epoll_fd_ = epoll_create(1);
    if (epoll_fd_ < 0) {
        std::cerr << "epoll_create error" << std::endl;
        exit(1);
    }
    events_.resize(max_events);
}

Epoll::~Epoll() {
    close(epoll_fd_);
}

bool Epoll::Add(int fd, uint32_t events){
    struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd , &event) < 0){
        std::cerr << "epoll_add error" << std::endl;
        return false;
    }
    return true;
}

bool Epoll::Modify(int fd , uint32_t events){
        struct epoll_event event;
    event.events = events;
    event.data.fd = fd;
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd , &event) < 0){
        std::cerr << "epoll_modify error" << std::endl;
        return false;
    }
    return true;
}

bool Epoll::Delete(int fd){
    if(epoll_ctl(epoll_fd_,EPOLL_CTL_DEL,fd,NULL) < 0){
        std::cerr << "epoll_delete error" << std::endl;
        return false;
    }
    return true;
}

int Epoll::Wait(int timeout){
    int num_events = epoll_wait(epoll_fd_,events_.data(),events_.size(),timeout);
    if(num_events < 0){
        std::cerr << "epoll_wait error" << std::endl;
        return -1;
    }
    return num_events;
}

