/* This component is for process http req and store content */

#include <iostream>

#include "Http_parser.h"
#include "Config.h"
#include "MsgQueue.h"
#include "Data_manager.h"
#include "QueryResult.h"
#include "Persistence_worker.h"

typedef MyMsgQueue<Q_LEN, ReqData> MsgQueue;
typedef MyMsgQueue<QueryResult_Q_LEN, QueryResult> ResultQueue;

ReqData req_data_buffer_instance;
ReqData* req_buffer = &req_data_buffer_instance;
int shm_id = 0;

MsgQueue *msg_queue;
ResultQueue *result_queue;

int Init_msg_queue(){
    void* shm_ptr = Get_shm(shm_id, sizeof(MsgQueue), server_shm_pathname);
    if(shm_ptr == nullptr){
        err_sys("shm_ptr is nullptr\n");
    }
    msg_queue = reinterpret_cast<MsgQueue* > (shm_ptr);
    result_queue = reinterpret_cast<ResultQueue* > (msg_queue + 1 );
    while(!msg_queue->isInit());
    while(!result_queue->isInit());
}

int Close_msg_queue(int shm_id){
    printf("log_processor ends\n");
}


void Append_req(ReqData* req_data)
{
    Append_log(req_data);
}

void Recent_req(ReqData* req_data)
{
    // printf("not surpported now\n");
    Get_recent_log(req_data);
    return;
}

void Getkey_req(ReqData* req_data)
{
    Getkey_log(req_data);
    return;
}

void Persistence_req(ReqData* req_data)
{
    Persistence_log(req_data);
    return;
}

void Search_req(ReqData* req_data)
{
    Search_log(req_data);
    return;
}

void SearchTime_req(ReqData* req_data)
{
    SearchTime_log(req_data);
    return;
}

void LogCount_req(ReqData* req_data)
{
    LogCount_log(req_data);
    return;
}

void Process_req(ReqData* req_data)
{
    if(req_data == nullptr || !req_data->Valid() ){
        printf("invalid req\n");
        req_data->Print();
        return;
    }

    switch(req_data->type_){
            case ReqType::Append:{
                Append_req(req_data);
                return;
            }
            case ReqType::Recent:{
                Recent_req(req_data);
                return;
            }
            case ReqType::Getkey:{
                Getkey_req(req_data);
                return;
            }
            case ReqType::Search:{
                Search_req(req_data);
                return;
            }
            case ReqType::SearchTime:{
                SearchTime_req(req_data);
                return;
            }
            case ReqType::Persistence:{
                Persistence_req(req_data);
                return;
            }
            case ReqType::LogCount:{
                LogCount_req(req_data);
                return;
            }
            case ReqType::Invalid:
            default:{
                return;
            }
    }
}

int main()
{
    int shm_id = Init_msg_queue();

    Init_data_manager();
    Persistence_worker_thread_init();

    while(1){
        int data_read = 0;

        while(msg_queue->Empty());
        memset(req_buffer, 0, sizeof(*req_buffer) );
        data_read = msg_queue->ReadAndDeleteFromMsgQueue(req_buffer);
        if(data_read == 0){
            continue;
        }

        // std::cout << "buf: " << req_buffer->content_ << std::endl;
        ReqData* req_data = req_buffer;

        Process_req(req_data);


    }

    Close_msg_queue(shm_id);
    End_data_manager();

    return 0;
}