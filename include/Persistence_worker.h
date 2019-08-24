/* This component is a persistence worker */

#ifndef PERSISTENCE_WORKER_H
#define PERSISTENCE_WORKER_H

#include <vector>

// #include "Data_manager.h"
#include "MsgQueue.h"
#include "TimerWheel.h"
#include "Config.h"

class MemoryBulkHandler;

typedef char BulkDataBuffer[BULKSIZE];
class BulkData
{
public:
    MemoryBulkHandler* bulk_handler_ = nullptr;
    BulkDataBuffer buffer_;
};
typedef MyMsgQueue<BULK_QUEUE_LEN, BulkData> BulkDataQueue;
extern BulkDataQueue bulk_data_queue;


extern std::vector<MemoryBulkHandler*> bulk_handler_list;


int Persistence_worker_thread_init();

class BulkTimeout:public TimerBase
{
public:
    MemoryBulkHandler* bulk_handler_;
    void Act();

    BulkTimeout(MemoryBulkHandler* bulk_handler);


    ~BulkTimeout(){}

};

extern TimerWheelSet<BulkTimeout> bulk_data_timer_wheel;
class BulkAutoPersistence : public TimerBase
{
public:
    MemoryBulkHandler* bulk_handler_;
    void Act();

    BulkAutoPersistence(MemoryBulkHandler* bulk_handler);


    ~BulkAutoPersistence(){}

};
extern TimerWheelSet<BulkAutoPersistence> bulk_auto_persistence_timer_wheel;


#endif