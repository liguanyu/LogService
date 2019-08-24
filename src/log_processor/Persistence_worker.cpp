#include <vector>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>
#include <stdio.h>
#include <thread>


#include "Persistence_worker.h"
#include "Data_manager.h"
#include "wrap.h"

#include "Config.h"

BulkDataQueue bulk_data_queue;
TimerWheelSet<BulkTimeout> bulk_data_timer_wheel;
TimerWheelSet<BulkAutoPersistence> bulk_auto_persistence_timer_wheel;

class PersistenceWorker
{
public:
    int Main_loop();
    int Check_bulk_data_queue_full();
};

void Sleep(int seconds, int mseconds)
{
    struct timeval temp;

    temp.tv_sec = seconds;
    temp.tv_usec = mseconds * 1000;

    select(0, NULL, NULL, NULL, &temp);

    return ;
};

int PersistenceWorker::Check_bulk_data_queue_full()
{
    // bulk_data_queue.Read_lock();
    if(bulk_data_queue.Size() >= PERSISTENCE_THRESHOLD){
        for(int i = 0; i < PERSISTENCE_THRESHOLD-1; i++)
        {
            BulkData* data = bulk_data_queue.Front_Unit();
            MemoryBulkHandler* bulk_handler = data->bulk_handler_;
            if(bulk_handler != nullptr){
                bulk_handler->Persistence();
                // bulk_handler->Delete_auto_persistence_timer();
            }
            bulk_data_queue.Release_front_unit();
        }
    }

    // bulk_data_queue.Read_unlock();
}


int PersistenceWorker::Main_loop()
{
    while(1){
        Sleep(0, PERSISTENCE_SLEEP_TIME);
        Check_bulk_data_queue_full();
        bulk_data_timer_wheel.Tick();
        bulk_auto_persistence_timer_wheel.Tick();

    }

}


int Persistence_worker_thread()
{
    PersistenceWorker worker;
    worker.Main_loop();
    err_sys("Nerver here\n");
}



int Persistence_worker_thread_init()
{
    std::thread t1(Persistence_worker_thread);
    t1.detach();
}


BulkTimeout::BulkTimeout(MemoryBulkHandler* bulk_handler)
    : bulk_handler_(bulk_handler)
{}

void BulkTimeout::Act(){
    bulk_handler_->Get_write_lock();
    bulk_handler_->Release_space();
    bulk_handler_->Drop_write_lock();
}

BulkAutoPersistence::BulkAutoPersistence(MemoryBulkHandler* bulk_handler)
    : bulk_handler_(bulk_handler)
{}

void BulkAutoPersistence::Act(){
    bulk_handler_->Persistence();
}