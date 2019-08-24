/* this component is for store and manage log data */

#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <map>
#include <deque>
#include <cstring>
#include <sstream>
#include <fstream>
#include <thread> 
#include <vector>
#include <sys/stat.h>  
#include <atomic>
#include <cstdint>

#include "MsgQueue.h"
#include "Data_manager.h"
#include "wrap.h"
#include "Trie.h"
#include "QueryResult.h"
#include "LogFile.h"
#include "Persistence_worker.h"

#include "Config.h"

#define KEY_OFFSET_MAP_UNORDERED 1

#define OPEN_LOG_FILE_MODE (std::ios::app|std::ios::out|std::ios::trunc)
const char delimiters[] = " ~`!@#$%^&*()+={}[]\":;,.<>?/\\\t\n";
const char invalid_answer[] = "Not a search req\n";

typedef MyMsgQueue<QueryResult_Q_LEN, QueryResult> ResultQueue;
extern ResultQueue* result_queue;


typedef Trie<MemoryBulkHandler*, false> KeyBulkHandlerTrie;
typedef std::vector<MemoryBulkHandler*> BulkHandlerList;
KeyBulkHandlerTrie Global_key_bulk_handler_trie;
void Add_key_global_trie(const char* key, MemoryBulkHandler* bulk_handler)
{
    BulkHandlerList *list = Global_key_bulk_handler_trie.Search(key);
    if(list == nullptr){
        Global_key_bulk_handler_trie.Insert(key, bulk_handler);
        return;
    }else if(list->size() > 0 && list->back() == bulk_handler){
        return;
    }else{
        list->push_back(bulk_handler);
    }
}
BulkHandlerList* Find_key_in_global_trie(const char* key)
{
    return Global_key_bulk_handler_trie.Search(key);
}


uint64_t LogSerial = 0;
uint64_t LogSerialAdd(uint64_t num)
{
    LogSerial += num;
    return LogSerial;
}



static int Find_char(const char* req_str, char c)
{
    int ret = 0;
    while(*req_str != c && *req_str != '\0' && *req_str != '\n'){
        req_str++;
        ret++;
    } 
    if(*req_str == c){
        return ret;
    }else{
        return -1;
    }
}



int MemoryBulk::Build_index(int offset, const char* content, const char* key, uint64_t timestamp)
{
    index_.key_serial_trie_.Insert(key, size_);

    char *str = new char[strlen(content) + 1];
    lgy_strncpy(str, content, strlen(content));
    char* saveptr;
    char* word;
    
    while(NULL != ( word = strtok_r( str, delimiters, &saveptr) ))
    {
        index_.keyword_serial_trie_.Insert(word, size_);
        str = NULL;
    }

    index_.offset_list_.push_back(offset);
    index_.timestamp_list_.push_back(timestamp);

    if(start_time_ > timestamp || start_time_ == 0){
        start_time_ = timestamp;
    }
    if(end_time_ < timestamp){
        end_time_ = timestamp;
    }

    size_++;
}

std::vector<MemoryBulkHandler*> bulk_handler_list;


// TODO, design a lock
void Get_bulk_handler_queue_write_lock()
{

}

void Release_bulk_handler_queue_write_lock()
{

}

void Get_bulk_handler_queue_read_lock()
{

}

void Release_bulk_handler_queue_read_lock()
{

}


void Init_data_manager()
{
    // key_record_map.reserve(MAP_INIT_NUM);
    // key_record_map.rehash(MAP_INIT_NUM);

    MemoryBulk* bulk_ptr = new MemoryBulk();
    MemoryBulkHandler* bulk_handler = new MemoryBulkHandler(bulk_ptr);
    bulk_handler_list.push_back(bulk_handler);
}


void End_data_manager()
{
    // while(!bulk_handler_list.empty()){
    //     MemoryBulk* bulk_ptr = bulk_handler_list.front();
    //     bulk_handler_list.pop_front();
    //     delete bulk_ptr;
    // }
}

MemoryBulkHandler* Get_available_memory_bulk();
MemoryBulkHandler* New_memory_bulk()
{
    // printf("New_memory_bulk\n");
    MemoryBulk* bulk;
    MemoryBulkHandler* bulk_handler;
    if(bulk_handler_list.empty())
    {
        bulk = new MemoryBulk(0);
        bulk_handler = new MemoryBulkHandler(bulk);
    }else{
        int start_serial = LogSerialAdd(Get_available_memory_bulk()->Get_size());
        bulk = new MemoryBulk(start_serial);
        bulk_handler = new MemoryBulkHandler(bulk);
    }

    bulk_handler_list.push_back(bulk_handler);
    return bulk_handler;
}   


MemoryBulkHandler* Get_available_memory_bulk()
{
    if(bulk_handler_list.empty())
    {
        New_memory_bulk();
    }
    return bulk_handler_list.back();
}



int MemoryBulk::Append(const char* str, int len){
    int old_offset_;
    if(offset_ + len + 1 >= BULKSIZE){
        return -1;
    }

    strncpy(data_->buffer_ + offset_, str, len);

    old_offset_ = offset_;

    offset_ += len;


    return old_offset_;
}

int MemoryBulkHandler::Append(const char* str, int len)
{
    if(state_ == MemoryBulkState::Appending){
        Get_write_lock();
        int ret = bulk_->Append(str, len);
        if(ret == -1){
            state_ = MemoryBulkState::InQueue;
        }
        Set_auto_persistence_timer();
        Drop_write_lock();
        return ret;
    }

    return -1;
}



int MemoryBulkHandler::Build_index(int offset, const char* content, const char* key, uint64_t timestamp)
{
    if(state_ != MemoryBulkState::Appending){
        return -1;
    }

    if(start_time_ > timestamp || start_time_ == 0){
        start_time_ = timestamp;
    }
    if(end_time_ < timestamp){
        end_time_ = timestamp;
    }

    return bulk_->Build_index(offset, content, key, timestamp);
}


int Append_log(ReqData* req_data)
{
    if(!req_data->Valid()){
        return -1;
    }

    MemoryBulkHandler* bulk_handler = Get_available_memory_bulk();

    std::stringstream ss;
    ss << "[key: " << req_data->key_
        << ", time: " << req_data->time_
        << "] " << req_data->content_ << std::endl;

    int offset = bulk_handler->Append(ss.str().c_str(), strlen(ss.str().c_str() ) );
    if( offset == -1 ){
        bulk_handler = New_memory_bulk();
        offset = bulk_handler->Append(ss.str().c_str(), strlen(ss.str().c_str() ) );
    }

    if(offset == -1){
        err_sys("some error, %s %s\n", __FUNCTION__, __LINE__);
    }

    bulk_handler->Build_index(offset, req_data->content_, req_data->key_, req_data->time_);
    Add_key_global_trie(req_data->key_, bulk_handler);

    return 1;
    
}

class LogHandler
{
public:
    MemoryBulkHandler* bulk_handler_ = nullptr;
    int local_serial_ = 0;
    int offset_ = 0;

    LogHandler(MemoryBulkHandler* bulk_handler, int local_serial) 
            : bulk_handler_(bulk_handler), local_serial_(local_serial) 
    {
        offset_ = bulk_handler_->Get_offset_by_serial(local_serial);
    }

    char* Get_content();
    const char* Get_content_ref(int &len);
};

// please remember to delete[]
char* LogHandler::Get_content(){
    const char* log_start = bulk_handler_->Get_data() + offset_;
    int len = Find_char(log_start, '\n');
    if(len == -1){
        err_sys("No~");
    }
    char* log = new char[len + 1];
    lgy_strncpy(log, log_start, len);
    return log;
}

const char* LogHandler::Get_content_ref(int &len){
    const char* log_start = bulk_handler_->Get_data() + offset_;
    len = Find_char(log_start, '\n');
    if(len == -1){
        err_sys("No~");
    }
    return log_start;
}


static uint64_t Get_time_microsec(){
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

class QueryResultHandler
{
public:
    QueryResult* result = nullptr;
    int client_id_;

    uint64_t start_time_ = 0;
    uint64_t end_time_ = 0;
    
    uint64_t count_ = 0;

    QueryResultHandler(int client_id) : client_id_(client_id){
        result = new QueryResult(client_id_);
        result->Set_begin();
        start_time_ = Get_time_microsec();
    }

    void Append(const char* log, size_t len){
        // +1 for '\n'
        int ret = result->Append(log, len + 1);
        if(ret == 0){
            while(result_queue->WriteToMsgQueue(result, sizeof(QueryResult) ) == 0);
            delete result;
            result = nullptr;
            result = new QueryResult(client_id_);
            ret = result->Append(log, len + 1);
        }
        count_++;
    }

    void Finish(){
        while(result_queue->WriteToMsgQueue(result, sizeof(QueryResult) ) == 0);
        delete result;
        result = nullptr;
        end_time_ = Get_time_microsec();
        result = new QueryResult(client_id_);
        std::stringstream ss;
        ss << "Return " << count_  <<" results use time " <<end_time_ - start_time_ << " microseconds " << std::endl;
        result->Append(ss.str().c_str(), ss.str().size() );
        result->Set_finish();
        while(result_queue->WriteToMsgQueue(result, sizeof(QueryResult) ) == 0);
        delete result;
        result = nullptr;
    }

    ~QueryResultHandler(){
        if(result != nullptr){
            delete result;
        }
    }
};


int Get_recent_log(ReqData* req_data)
{
    printf("Get_recent_log\n");    
    QueryResultHandler result_handler(req_data->client_id_);
    if(!req_data->Valid() || req_data->type_ != ReqType::Recent){
        printf("Not a search req\n");
        result_handler.Append(invalid_answer, strlen(invalid_answer));
        result_handler.Finish();
    }

    int recent_count = req_data->count_;

    int num = recent_count;
    auto r_iter = bulk_handler_list.rbegin();
    for (; r_iter != bulk_handler_list.rend() && num > 0; ++r_iter)
    {
        MemoryBulkHandler* bulk_handler = *r_iter;
        bulk_handler->Get_read_lock();
        
        int start_index = bulk_handler->bulk_->start_serial_;
        int end_index = bulk_handler->Get_size() - 1;

        for (; end_index >= 0 && num > 0; end_index--, num--){
            LogHandler log_handler(bulk_handler, end_index);
            int len = 0;
            const char* log = log_handler.Get_content_ref(len);
            result_handler.Append(log, len);
        }

        bulk_handler->Drop_read_lock();
    }

    result_handler.Finish();


    return 1;

}



// new a thread to do Persistence
// void Thread_do_persistence(std::vector<MemoryBulkHandler*> *bulk_l)
// {

//     for(MemoryBulkHandler* bulk_handler : *bulk_l)
//     {
//         printf("persistence %d\n", bulk_handler->Get_size());
//         bulk_handler->Persistence();
//         // delete bulk_handler;
//     }

//     // delete bulk_l;

// }



const char persistence_ret[] = "Persistence down\n";
// TODO: now only for single writer thread
int Persistence_log(ReqData* req_data)
{
    // Get_bulk_handler_queue_write_lock();
    printf("Persistence_log\n");

    // std::vector<MemoryBulkHandler*> *new_bulk_handler_list = new std::vector<MemoryBulkHandler*>();
    // new_bulk_handler_list->swap(bulk_handler_list);

    // // log_record_list = &log_record_list_root;
    // // log_record_list->next_ = nullptr;

    // std::thread t1(Thread_do_persistence, &bulk_handler_list);
    // t1.detach();

    QueryResultHandler result_handler(req_data->client_id_);
    result_handler.Append(persistence_ret, strlen(persistence_ret));
    result_handler.Finish();

    // Release_bulk_handler_queue_write_lock();
}


std::vector<std::string> Split_key_word(const char* keyword_content)
{
    std::vector<std::string> keyword_list;

    char *str = new char[strlen(keyword_content) + 1];
    lgy_strncpy(str, keyword_content, strlen(keyword_content));
    char* saveptr;
    char* word;
    
    while(NULL != ( word = strtok_r( str, delimiters, &saveptr) ))
    {
        std::string keyword = word;
        keyword_list.push_back(keyword);
        str = NULL;
    }

    return keyword_list;
}


int Find_smallest_iterator(std::vector< std::vector<int>::iterator > &iterator_list)
{
    int num =  iterator_list.size();

    int smallest = 0;
    int smallest_serial = (*iterator_list[0]);
    for(int i = 1; i < num; i++){
        if( (*iterator_list[i]) < smallest_serial){
            smallest = i;
            smallest_serial = (*iterator_list[i]);
        }
    }

    return smallest;
}


bool Are_the_iterator_serial_equal(std::vector< std::vector<int>::iterator > &iterator_list)
{
    int result = true;
    int serial = *iterator_list[0];

    for(auto &iter : iterator_list){
        if( (*iter) != serial){
            result = false;
            break;
        }
    }

    return result;
}



std::vector<int> MemoryBulk::Join_search(std::vector<std::string> &word_list)
{
    std::vector< std::vector<int>* > result_for_each_word;
    std::vector<int> result;

    int number_of_keyword = word_list.size();
    std::vector< std::vector<int>::iterator > iterator_list;

    for(std::string &keyword : word_list){
        std::vector<int >* tmp = Search_keyword(keyword.c_str() );
        if(tmp == nullptr){
            return result;
        }
        result_for_each_word.push_back(tmp);
    }

    for(int i = 0; i < number_of_keyword; i++){
        iterator_list.push_back(result_for_each_word[i]->begin() );
    }

    while(1){
        bool end = 0;
        for(int i = 0; i < number_of_keyword; i++){
            if(iterator_list[i] == result_for_each_word[i]->end() ){
                end = 1;
                break;
            }
        }
        if(end == 1){
            break;
        }

        if(Are_the_iterator_serial_equal(iterator_list)){
            result.push_back(*(iterator_list[0]) );
            for(auto &iter : iterator_list){
                iter++;
            }
            continue;
        }

        int smallest = Find_smallest_iterator(iterator_list);
        iterator_list[smallest]++;
    }

    return result;
}



int Search_log(ReqData* req_data)
{
    printf("Search_log\n");
    QueryResultHandler result_handler(req_data->client_id_);
    if(!req_data->Valid() || req_data->type_ != ReqType::Search){
        printf("Not a search req\n");
        result_handler.Append(invalid_answer, strlen(invalid_answer));
        result_handler.Finish();
        return -1;
    }


    std::vector<std::string> keyword_list = Split_key_word(req_data->keyword_);


    int num = MAX_NUM_RESULT;
    if(keyword_list.size() == 1){
        auto list_it = bulk_handler_list.rbegin();
        for(;list_it != bulk_handler_list.rend() && num >= 0; list_it++){
            MemoryBulkHandler* bulk_handler = *list_it;
            bulk_handler->Get_read_lock();
            
            std::vector<int>* tmp_serial_list; 
            tmp_serial_list = bulk_handler->Search_keyword(req_data->keyword_);

            if(tmp_serial_list != nullptr){
                auto serial_it = tmp_serial_list->rbegin();
                for(;serial_it != tmp_serial_list->rend() && num >= 0; tmp_serial_list++){
                    int serial = *serial_it;
                    LogHandler log_handler(bulk_handler, serial);
                    int len = 0;
                    const char* log = log_handler.Get_content_ref(len);
                    result_handler.Append(log, len);
                    num--;
                }
            }

            bulk_handler->Drop_read_lock();
            
        }
    }
    else if(keyword_list.size() > 1){
        auto list_it = bulk_handler_list.rbegin();
        for(;list_it != bulk_handler_list.rend() && num >= 0; list_it++){
            MemoryBulkHandler* bulk_handler = *list_it;
            bulk_handler->Get_read_lock();

            std::vector<int> result = bulk_handler->Join_search(keyword_list);
            if(result.size() != 0){
                auto serial_it = result.rbegin();
                for(;serial_it != result.rend() && num >= 0; serial_it++){
                    int serial = *serial_it;
                    LogHandler log_handler(bulk_handler, serial);
                    int len = 0;
                    const char* log = log_handler.Get_content_ref(len);
                    result_handler.Append(log, len);
                    num--;
                }
            }


            bulk_handler->Drop_read_lock();

        }
    }

    if(num == 0){
        const char ret[] = "No result found\n";
        result_handler.Append(ret, strlen(ret));
    }

    result_handler.Finish();

    return num;
}



// TODO: send back
int Getkey_log(ReqData* req_data)
{
    printf("Getkey_log\n");
    QueryResultHandler result_handler(req_data->client_id_);
    if(!req_data->Valid() || req_data->type_ != ReqType::Getkey){
        printf("Not a getkey req\n");
        result_handler.Append(invalid_answer, strlen(invalid_answer));
        result_handler.Finish();
        return -1;
    }


    int num = MAX_NUM_RESULT;
    BulkHandlerList *list = Find_key_in_global_trie(req_data->key_);
    if(list != nullptr){
        auto list_it = list->rbegin();
        for(;list_it != list->rend() && num >= 0; list_it++){

            MemoryBulkHandler* bulk_handler = *list_it;
            bulk_handler->Get_read_lock();
            std::vector<int>* tmp_serial_list; 
            tmp_serial_list = bulk_handler->Search_key(req_data->key_);
            
            if(tmp_serial_list != nullptr){
                auto serial_it = tmp_serial_list->rbegin();
                for(;serial_it != tmp_serial_list->rend() && num >= 0; serial_it++){
                    int serial = *serial_it;
                    LogHandler log_handler(bulk_handler, serial);
                    int len = 0;
                    const char* log = log_handler.Get_content_ref(len);
                    result_handler.Append(log, len);
                    num--;
                }
            }

            bulk_handler->Drop_read_lock();

        }
    }

    if(num == 0){
        const char ret[] = "No result found\n";
        result_handler.Append(ret, strlen(ret));
    }


    result_handler.Finish();

    return 1;

}

int Find_start_time_serial(uint64_t start_time, OffsetList* offset_list
                            , const TimeStampList &timestamp_list)
{
    OffsetList &array = *offset_list;
    int n = offset_list->size();
    if (n == 0) {
        return -1;
    }   
    if(timestamp_list[array[0]] >= start_time){
        return 0;
    }
    if(timestamp_list[array[n-1]] < start_time){
        return -1;
    }   
    int left = 0;
    int right = n - 1;  
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (timestamp_list[array[mid]] >= start_time) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }
    return left;

}

int Find_end_time_serial(uint64_t end_time, OffsetList* offset_list
                            , const TimeStampList &timestamp_list)
{
    OffsetList &array = *offset_list;
    int n = offset_list->size();
    if (n == 0) {
        return -1;
    }   
    if(timestamp_list[array[0]] > end_time){
        return -1;
    }
    if(timestamp_list[array[n-1]] <= end_time){
        return n-1;
    }   
    int left = 0;
    int right = n - 1;  
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (timestamp_list[array[mid]] > end_time) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }
    return right;

}



OffsetList* MemoryBulk::Search_time_key(uint64_t start, uint64_t end, const char* key
                , int &start_index, int &end_index, bool &end_search)
{
    if(start > end_time_){
        end_search = true;
        return nullptr;
    }

    if(end < start_time_){
        return nullptr;
    }

    OffsetList* offset_list = Search_key(key);
    if(offset_list == nullptr){
        return nullptr;
    }


    start_index = Find_start_time_serial(start, offset_list, index_.timestamp_list_);
    end_index = Find_end_time_serial(end, offset_list, index_.timestamp_list_);

    if(start_index == -1 || end_index == -1){
        return nullptr;
    }
    if(start_index > end_index){
        return nullptr;
    }

    return offset_list;
}



int Find_start_time_bulkhandler(uint64_t start_time, BulkHandlerList* list)
{
    BulkHandlerList &array = *list;
    int n = array.size();
    if (n == 0) {
        return -1;
    }   
    if(array[0]->end_time_ > start_time){
        return 0;
    }
    if(array[n-1]->end_time_ < start_time){
        return n-1;
    }   
    int left = 0;
    int right = n - 1;  
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (array[mid]->end_time_ > start_time) {
            right = mid - 1;
        }
        else {
            left = mid + 1;
        }
    }
    return right;

}


int SearchTime_log(ReqData* req_data)
{
    printf("SearchTime_log\n");
    QueryResultHandler result_handler(req_data->client_id_);
    if(!req_data->Valid() || req_data->type_ != ReqType::SearchTime){
        printf("Not a search key req\n");
        result_handler.Append(invalid_answer, strlen(invalid_answer));
        result_handler.Finish();
        return -1;
    }
    int start_time = req_data->start_;
    int end_time = req_data->end_;


    BulkHandlerList *list = Find_key_in_global_trie(req_data->key_);
    int num = 0;
    bool end_search = false;
    if(list != nullptr){
        int start_bulk_index = Find_start_time_bulkhandler(start_time, list);    
        int list_len = list->size();    

        for(int index = start_bulk_index; index < list_len; index++){
            MemoryBulkHandler *bulk_handler = (*list)[index];
            bulk_handler->Get_read_lock();
            std::vector<int>* tmp_serial_list; 
            int start_index, end_index;
            tmp_serial_list = bulk_handler->Search_time_key(start_time, end_time, req_data->key_
                        , start_index, end_index, end_search);
            
            if(!end_search){
                if(tmp_serial_list != nullptr){
                    // for(int serial : *tmp_serial_list){
                    //     LogHandler log_handler(bulk_handler, serial);
                    //     int len = 0;
                    //     const char* log = log_handler.Get_content_ref(len);
                    //     result_handler.Append(log, len);
                    //     num++;

                    for(int i = start_index; i <= end_index; i++){
                        LogHandler log_handler(bulk_handler, (*tmp_serial_list)[i]);
                        int len = 0;
                        const char* log = log_handler.Get_content_ref(len);
                        result_handler.Append(log, len);
                        num++;
                    }
                }
            }

            bulk_handler->Drop_read_lock();

        }
    }

    if(num == 0){
        const char ret[] = "No result found\n";
        result_handler.Append(ret, strlen(ret));
    }


    result_handler.Finish();

    return 1;

}


int LogCount_log(ReqData* req_data)
{
    printf("LogCount_log\n");
    QueryResultHandler result_handler(req_data->client_id_);
    if(!req_data->Valid() || req_data->type_ != ReqType::LogCount){
        printf("Not a logcount key req\n");
        result_handler.Append(invalid_answer, strlen(invalid_answer));
        result_handler.Finish();
        return -1;
    }

    MemoryBulkHandler* bulk_handler = bulk_handler_list.back();
    uint64_t count = bulk_handler->bulk_->start_serial_ + bulk_handler->bulk_->size_;

    std::stringstream ss;
    ss << "Now record log count is " << count << std::endl;
    

    result_handler.Append(ss.str().c_str(), ss.str().size() );

    result_handler.Finish();

    return 1;
}