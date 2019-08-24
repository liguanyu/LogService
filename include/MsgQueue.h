#ifndef MYMSGQUEUE_H
#define MYMSGQUEUE_H

#include <iostream>
#include <string>
#include <sys/sem.h>
#include "wrap.h"
#include <cstring>

// TODO: it seems the sem has some problems

template<int QUEUE_LEN, typename T>
class MyMsgQueue{
public:
    int size_;
    int one_msg_size_;

    int write_position_ = 0;
    int read_position_ = 0;

    sem_t read_sem_;
    sem_t write_sem_;

    char msg_queue_[QUEUE_LEN * sizeof(T)];

    int WriteToMsgQueue(const T* buf, int len = 0);
    int WriteToMsgQueue_Wait(const T* buf, int len = 0);
    int ReadAndDeleteFromMsgQueue(T* buf);

    T* Get_Unit();
    T* Front_Unit();
    T* Release_front_unit();

    MyMsgQueue(){
        size_ = QUEUE_LEN;
        one_msg_size_ = sizeof(T);
        Sem_init(&read_sem_, 1, 1);
        Sem_init(&write_sem_, 1, 1);
    }

    void Read_lock() {sem_wait(&read_sem_);}
    void Read_unlock() {sem_post(&read_sem_);}
    
    bool isInit() {return size_ != 0;}
    bool Empty() {return write_position_ == read_position_;}
    bool Full() {return (write_position_ + 1) % size_ == read_position_;}
    int Size() {
        if(write_position_ >= read_position_){
            return write_position_ - read_position_;
        }
        return write_position_ + size_ - read_position_;
    }

};

template<int QUEUE_LEN, typename T>
int MyMsgQueue<QUEUE_LEN, T>::WriteToMsgQueue(const T* buf, int len)
{
    // sem_wait(&write_sem_);

    if(len == 0 || len > one_msg_size_){
        len = one_msg_size_;
    }

    if(Full())
    {
        sem_post(&write_sem_);
        return 0;
    }

    // std::cout<<buf<<std::endl;
    assert(len <= one_msg_size_);
    if(buf != nullptr){
        memcpy(msg_queue_ + one_msg_size_ * write_position_, buf, len);
    }

    write_position_++;
    if(write_position_ >= size_){
        write_position_ = 0;
    }
    
    // sem_post(&write_sem_);
    return len;
}

template<int QUEUE_LEN, typename T>
int MyMsgQueue<QUEUE_LEN, T>::WriteToMsgQueue_Wait(const T* buf, int len)
{
    // sem_wait(&write_sem_);

    if(len == 0 || len > one_msg_size_){
        len = one_msg_size_;
    }

    while(Full());

    // std::cout<<buf<<std::endl;
    assert(len <= one_msg_size_);
    if(buf != nullptr){
        memcpy(msg_queue_ + one_msg_size_ * write_position_, buf, len);
    }

    write_position_++;
    if(write_position_ >= size_){
        write_position_ = 0;
    }
    
    // sem_post(&write_sem_);
    return len;
}



template<int QUEUE_LEN, typename T>
int MyMsgQueue<QUEUE_LEN, T>::ReadAndDeleteFromMsgQueue(T* buf)
{
    sem_wait(&read_sem_);

    if(Empty()){
        sem_post(&read_sem_);
        return 0;
    }

    // TODO: may overflow buf
    if(buf != nullptr){
        memcpy(buf, msg_queue_ + one_msg_size_ * read_position_, one_msg_size_);
    }
    read_position_++;
    if(read_position_ >= size_){
        read_position_ = 0;
    }

    sem_post(&read_sem_);
    return one_msg_size_;
}




template<int QUEUE_LEN, typename T>
T* MyMsgQueue<QUEUE_LEN, T>::Get_Unit()
{
    // sem_wait(&write_sem_);

    if(Full())
    {
        sem_post(&write_sem_);
        return nullptr;
    }

    memset(msg_queue_ + one_msg_size_ * write_position_, 0, one_msg_size_);
    T* ret = reinterpret_cast<T*>(msg_queue_ + one_msg_size_ * write_position_);

    write_position_++;
    if(write_position_ >= size_){
        write_position_ = 0;
    }
    
    // sem_post(&write_sem_);
    return ret;    
}

template<int QUEUE_LEN, typename T>
T* MyMsgQueue<QUEUE_LEN, T>::Front_Unit()
{
    sem_wait(&read_sem_);

    if(Empty()){
        sem_post(&read_sem_);
        return nullptr;
    }

    T* ret = reinterpret_cast<T*>(msg_queue_ + one_msg_size_ * read_position_);

    sem_post(&read_sem_);
    return ret;
}

template<int QUEUE_LEN, typename T>
T* MyMsgQueue<QUEUE_LEN, T>::Release_front_unit()
{
    sem_wait(&read_sem_);

    if(Empty()){
        sem_post(&read_sem_);
        return nullptr;
    }

    T* ret = reinterpret_cast<T*>(msg_queue_ + one_msg_size_ * read_position_);

    read_position_++;    
    if(read_position_ >= size_){
        read_position_ = 0;
    }
    sem_post(&read_sem_);
    return ret;
}


#endif