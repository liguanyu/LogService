#ifndef TIMEWHEEL_H
#define TIMEWHEEL_H

#include "Config.h"

template<class T>
class TimerUnit;

template<class T>
class TimerSlot;

// for generate Timer
class TimerBase{
private:
    void* timer_unit_p = nullptr;
public:
    virtual void Act() = 0;

    virtual void Set_timer_unit(void* p) final
    {
        timer_unit_p = p;
    }

    virtual void* Get_timer_unit() final
    {
        return timer_unit_p;
    }
};

template<class T>
class TimerUnit{
public:
    T* member_ = nullptr;
    int time_ = 0;
    int past_time_ = 0;

    TimerUnit(T* member, int time): member_(member), time_(time)
    {
        member_ -> Set_timer_unit(reinterpret_cast<void*>(this));
    }

    TimerSlot<T>* location_ = nullptr; 

    TimerUnit<T>* next_ = nullptr;
    TimerUnit<T>* prev_ = nullptr;

    void Act(){
        member_->Act();
        member_->Set_timer_unit(nullptr);

    }

    void SetLocation(TimerSlot<T>* location){
        location_ = location;
    }

    TimerSlot<T>* GetLocation(){
        return location_;
    }

    void ClearLocation(){
        location_ = nullptr;
    }

    ~TimerUnit(){
        // delete member_;
        member_->Set_timer_unit(nullptr);
    }

};


template<class T>
class TimerSlot{
public:
    TimerUnit<T>* timer_list_ = nullptr;

    bool Empty()
    {
        return timer_list_ == nullptr;
    }

    void Insert(TimerUnit<T>* timer_unit_p)
    {
        if(!Empty()){
            timer_list_->prev_ = timer_unit_p;
        }
        timer_unit_p->next_ = timer_list_;
        timer_list_ = timer_unit_p;
    }

    TimerUnit<T>* Pop()
    {
        TimerUnit<T>* tmp = timer_list_;
        if(!Empty()){
            timer_list_ = timer_list_->next_;
            if(timer_list_ != nullptr){
                timer_list_->prev_ = nullptr;
            }
            tmp->next_ = nullptr;
            tmp->ClearLocation();
        }
        return tmp;
    }

    void Delete(TimerUnit<T>* timer_unit_p)
    {
        if(timer_unit_p->prev_){
            timer_unit_p->prev_->next_ = timer_unit_p->next_;
        }
        if(timer_unit_p->next_){
            timer_unit_p->next_->prev_ = timer_unit_p->prev_;
        }

        if(timer_list_ == timer_unit_p)
            timer_list_ = timer_list_->next_;

        timer_unit_p->ClearLocation();

        delete timer_unit_p;
    }


};

template<class T, int N>
class TimerWheel{
public:
    TimerWheel(int interval) 
        : interval_(interval), current_index_(0), next_timer_wheel_(nullptr){}
    TimerWheel(int interval, TimerWheel<T, N>* next_timer_wheel, TimerWheel<T, N>* prev_timer_wheel) 
        : interval_(interval), current_index_(0), 
            next_timer_wheel_(next_timer_wheel), prev_timer_wheel_(prev_timer_wheel){}

    TimerSlot<T> slot_list_[N];
    int current_index_ = 0;

    int interval_;

    void AddTimer(T* obj, int time);
    void AddTimer(TimerUnit<T>* timer_unit_p);

    void Tick();
    
    TimerWheel<T, N>* next_timer_wheel_;
    TimerWheel<T, N>* prev_timer_wheel_;

    void DelTimer(T* obj);

};

// 50ms, 50s
template<class T>
class TimerWheelSet{
public:
    TimerWheel<T, 20> timer_wheel_1__;  // 50ms
    TimerWheel<T, 20> timer_wheel_10__;   // 500ms
    TimerWheel<T, 20> timer_wheel_100__;  // 5s

    TimerWheelSet()
        :   timer_wheel_1__(PERSISTENCE_SLEEP_TIME, &timer_wheel_10__, nullptr),
            timer_wheel_10__(PERSISTENCE_SLEEP_TIME * 20,  &timer_wheel_100__, &timer_wheel_1__),
            timer_wheel_100__(PERSISTENCE_SLEEP_TIME * 20 * 20, nullptr, &timer_wheel_10__ )
    {}

    void Tick();
    void AddTimer(T* obj, int time);
    void DelTimer(T* obj);

};

template<class T, int N>
void TimerWheel<T, N>::AddTimer(T* obj, int time)
{
    if(time <= 0){
        return;
    }

    TimerUnit<T>* timer_unit_p = new TimerUnit<T>(obj, time);

    if(time - timer_unit_p->past_time_  < N*interval_ || next_timer_wheel_ == NULL){
        int slot_offset = (time - timer_unit_p->past_time_ )/interval_;
        if(slot_offset >= N){
            slot_offset = N-1;
        }else if(slot_offset < 1){
            slot_offset = 1;
        }
        int insert_index = (current_index_ + slot_offset) % N;

        slot_list_[insert_index].Insert(timer_unit_p);
        timer_unit_p->SetLocation(&slot_list_[insert_index]);
    }else if(next_timer_wheel_){
        timer_unit_p->past_time_ += (N-current_index_) * interval_;
        next_timer_wheel_->AddTimer(timer_unit_p);
    }else{
        printf("sth wrong\n");
    }
}

template<class T, int N>
void TimerWheel<T, N>::AddTimer(TimerUnit<T>* timer_unit_p)
{
    // printf("Add timer past time %d, interval %d\n", timer_unit_p->past_time_, interval_);
    int time = timer_unit_p->time_;
    int past_time_ = timer_unit_p->past_time_;
    if(time - past_time_  < N*interval_ || next_timer_wheel_ == NULL){
        int slot_offset = (time - past_time_ ) / interval_;
        if(slot_offset >= N){
            slot_offset = N-1;
        }else if(slot_offset < 1){
            slot_offset = 1;
        }
        int insert_index = (current_index_ + slot_offset) % N;

        slot_list_[insert_index].Insert(timer_unit_p);
        timer_unit_p->SetLocation(&slot_list_[insert_index]);

        timer_unit_p->past_time_ += (slot_offset-1) * interval_;
    }else if(next_timer_wheel_){
        timer_unit_p->past_time_ += (N-current_index_) * interval_;
        next_timer_wheel_->AddTimer(timer_unit_p);
    }else{
        printf("sth wrong\n");
    }

}


template<class T, int N>
void TimerWheel<T, N>::Tick()
{
    // printf("tick, %d\n", interval_);
 
    int carry  = 0;
    current_index_ = (current_index_ + 1 ) % N;
    if(current_index_ == 0){
        carry = 1;
    }

    if(!prev_timer_wheel_){
        TimerUnit<T>* timer_unit_p = nullptr;
        while(timer_unit_p = slot_list_[current_index_].Pop()){
            timer_unit_p->Act();
            delete timer_unit_p;
        }
    }else{
        TimerUnit<T>* timer_unit_p;
        while(timer_unit_p = slot_list_[current_index_].Pop()){
            // printf("move\n");
            if(timer_unit_p->time_ - timer_unit_p->past_time_ <= 0 ||
                ((timer_unit_p->time_ - timer_unit_p->past_time_) % (N*interval_) ) == 0){
                timer_unit_p->Act();
                delete timer_unit_p;
            }else{
                prev_timer_wheel_->AddTimer(timer_unit_p);
            }
        }
    }


    if(carry == 1 && next_timer_wheel_ != NULL){
        next_timer_wheel_->Tick();
    }

}


template<class T, int N>
void TimerWheel<T, N>::DelTimer(T* obj)
{
    TimerUnit<T>* timer_unit_p = reinterpret_cast<TimerUnit<T>*>(obj->Get_timer_unit());
    if(timer_unit_p != nullptr){
        auto location = timer_unit_p->GetLocation();
        if(location != nullptr)
        {
            location->Delete(timer_unit_p);
        }
    }
}

template<class T>
void TimerWheelSet<T>::Tick()
{
    timer_wheel_1__.Tick();
}

template<class T>
void TimerWheelSet<T>::AddTimer(T* obj, int time)
{
    timer_wheel_1__.AddTimer(obj, time);
}

template<class T>
void TimerWheelSet<T>::DelTimer(T* obj)
{
    TimerUnit<T>* timer_unit_p = reinterpret_cast<TimerUnit<T>*>(obj->Get_timer_unit());
    if(timer_unit_p != nullptr){
        auto location = timer_unit_p->GetLocation();
        if(location != nullptr)
        {
            location->Delete(timer_unit_p);
        }
    }
}

#endif