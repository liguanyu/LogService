#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>          // for epoll
#include <cstdint>
#include <set>

#include "Config.h"
#include "wrap.h"


#define READ_MODE (EPOLLIN|EPOLLET|EPOLLRDHUP)
#define WRITE_MODE (EPOLLOUT|EPOLLET)


class Epoll_Obj{
public:
    static const int MAX_EVENT_NUMBER = 1024;
    int epoll_fd_;
    int listen_fd_;
    epoll_event events_[ MAX_EVENT_NUMBER ];
    int size_ = 0;

    std::set<int> fd_set_;

    Epoll_Obj(){
        epoll_fd_ = epoll_create( MAX_EVENT_NUMBER );
    }

    void AddListenFd(int listen_fd){
        AddFd(listen_fd);
        listen_fd_ = listen_fd;
    }

    void AddFd(int fd, uint32_t events = READ_MODE)
    {
        epoll_event event;
        event.data.fd = fd;
        // event.events = EPOLLIN | EPOLLET;
        event.events = events;
        if(epoll_ctl( epoll_fd_, EPOLL_CTL_ADD, fd, &event ) == -1){
            err_sys("epoll_ctl add error : %d\n", errno);
        }
        fd_set_.insert(fd);
        size_++;
        // setnonblocking( fd );
    }

    void DelFd(int fd)
    {
        epoll_event event;
        event.data.fd = fd;
        if(epoll_ctl( epoll_fd_, EPOLL_CTL_DEL, fd, &event ) == -1){
            err_sys("epoll_ctl del error : %d\n", errno);
        }
        fd_set_.erase(fd);
        size_--;
    } 

    int Epoll_Wait(int time = epoll_wait_time){
        int ret = epoll_wait( epoll_fd_, events_, MAX_EVENT_NUMBER, time );
        if(ret == -1){
            printf("epoll_wait error : %d\n", errno);
        }
        return ret;
    }

    void ModifyFdMode(int fd, uint32_t events){
        epoll_event event;
        event.data.fd = fd;
        event.events = events;
        if(epoll_ctl( epoll_fd_, EPOLL_CTL_MOD, fd, &event ) == -1){
            err_sys("ModifyFdMode error : %d\n", fd);
        }
    }

    void SetRead(int fd){
        ModifyFdMode(fd, READ_MODE);
    }

    void SetWrite(int fd){
        ModifyFdMode(fd, WRITE_MODE);
    }

    bool IsWaited(int fd){
        auto iter = fd_set_.find(fd);
        return iter != fd_set_.end();
    }
};


#endif