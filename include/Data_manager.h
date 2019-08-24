/* this component is for store and manage log data */

#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <cstdint>
#include <condition_variable>  

#include "Http_parser.h"
#include "MsgQueue.h"
#include "Trie.h"
#include "LogFile.h"
#include "MyType.h"
#include "Persistence_worker.h"
#include "RWLock.h"

void Init_data_manager();
void End_data_manager();

int Append_log(ReqData* req_data);
int Persistence_log(ReqData* req_data);
int Get_recent_log(ReqData* req_data);
int Getkey_log(ReqData* req_data);
int Search_log(ReqData* req_data);
int SearchTime_log(ReqData* req_data);
int LogCount_log(ReqData* req_data);


class MemoryBulk;
class MemoryBulkHandler;



class BulkData;
class BulkIndex
{
public:
    KeyTrie key_serial_trie_;

    KeyWordTrie keyword_serial_trie_;

    OffsetList offset_list_;

    TimeStampList timestamp_list_;

    bool valid_ = false;

    bool Valid() {return valid_;}

    void Set_valid(){
        valid_ = true;
    }

    void Set_unvalid(){
        valid_ = false;
    }
};

class MemoryBulk
{
public:
    uint64_t start_serial_ = 0;
    uint64_t start_time_ = 0;
    uint64_t end_time_ = 0;
    int size_ = 0;

    BulkIndex index_;

    int offset_ = 0;
    BulkData* data_ = nullptr;

    MemoryBulk(int start_serial = 0) : start_serial_(start_serial) {}

    ~MemoryBulk(){
        // printf("I am deleted\n");
    }

    int Append(const char* str, int len);

    const char* Get_data()
    {
        return reinterpret_cast<const char*> (data_->buffer_);
    }

    int Get_offset_by_serial(int serial){
        if(serial >= size_ ){
            return -1;
        }

        return index_.offset_list_[serial];
    }

    OffsetList* Search_keyword(const char* keyword){
        return index_.keyword_serial_trie_.Search(keyword);
    }

    OffsetList* Search_key(const char* key){
        return index_.key_serial_trie_.Search(key);
    }

    int Build_index(int offset, const char* content, const char* key, uint64_t timestamp);

    OffsetList Join_search(std::vector<std::string> &word_list);

    OffsetList* Search_time_key(uint64_t start, uint64_t end, const char* key
                    , int &start_index, int &end_index, bool &end_search);




};

class MemoryBulkHandler
{
public:
    enum class MemoryBulkState{
        Init = 0,
        Appending,
        InQueue,
        InMemory,
        Saved,
    } state_;

    struct IndexState{
        // Saved = 0,
        // OffsetRead,
        // KeyRead,
        // KeywordRead,
        // TimeStampRead,
        // InMemory

        bool offset_read_ = true;;
        bool timestamp_read_ = true;
        bool key_read_ = true;
        bool word_read_ = true;

    } index_state_;

    MemoryBulk* bulk_ = nullptr;

    LogContentFile content_file_;
    LogIndexFile index_file_;
    LogTimeStampFile timestamp_file_;
    LogKeyTrieFile key_file_;
    LogWordTrieFile word_file_;


    uint64_t start_time_ = 0;
    uint64_t end_time_ = 0;

    MemoryBulkHandler(MemoryBulk* bulk)
            : bulk_(bulk)
            , content_file_(bulk->start_serial_)
            , index_file_(bulk->start_serial_)
            , timestamp_file_(bulk->start_serial_)
            , key_file_(bulk->start_serial_)
            , word_file_(bulk->start_serial_)
            , bulk_timeout_(this)
            , bulk_auto_persistence_(this)
    {
        state_ = MemoryBulkState::Init;
        New_BulkData_init();
    }

    ~MemoryBulkHandler(){
        if(bulk_ != nullptr){
            Delete_Content();
            delete bulk_;
        }
    }

    void New_BulkData_init();
    void New_BulkData();

    BulkIndex* Get_index(){
        return &bulk_->index_;
    }

    int Get_size(){
        return bulk_->size_;
    }

    int Build_index(int offset, const char* content, const char* key, uint64_t timestamp);
    int Append(const char* str, int len);
    int Persistence();

    int Save_Content();
    int Save_index();
    int Save_timestamp();
    int Save_key_trie();
    int Save_word_trie();

    int Read_Content();
    int Read_index();
    int Read_timestamp();
    int Read_key_trie();
    int Read_word_trie();

    int Delete_Content();
    int Delete_index();
    int Delete_timestamp();
    int Delete_key_trie();
    int Delete_word_trie();

    int Release_space()
    {
        Delete_Content();
        Delete_index();
        Delete_timestamp();
        Delete_key_trie();
        Delete_word_trie();
    }

    BulkTimeout bulk_timeout_;
    void Set_timer(){
        if(state_ == MemoryBulkState::Appending || state_ == MemoryBulkState::InQueue){
            return;
        }
        bulk_data_timer_wheel.DelTimer(&bulk_timeout_);
        bulk_data_timer_wheel.AddTimer(&bulk_timeout_, RELEASE_TIME);
    }

    BulkAutoPersistence bulk_auto_persistence_;
    void Set_auto_persistence_timer(){
        if(state_ != MemoryBulkState::Appending){
            return;
        }
        bulk_auto_persistence_timer_wheel.DelTimer(&bulk_auto_persistence_);
        bulk_auto_persistence_timer_wheel.AddTimer(&bulk_auto_persistence_, AUTOSAVE_TIME);
    }
    void Delete_auto_persistence_timer(){
        bulk_auto_persistence_timer_wheel.DelTimer(&bulk_auto_persistence_);
    }

    RWLock lock_;
    void Get_read_lock(){
        lock_.getReadLock();
    }

    void Get_write_lock(){
        lock_.getWriteLock();
    }

    void Drop_read_lock(){
        lock_.unlockReader();
    }

    void Drop_write_lock(){
        lock_.unlockWriter();
    }
    

    const char* Get_data();

    int Get_offset_by_serial(int serial);

    OffsetList* Search_keyword(const char* keyword);

    OffsetList* Search_key(const char* key);

    OffsetList Join_search(std::vector<std::string> &word_list);

    OffsetList* Search_time_key(uint64_t start, uint64_t end, const char* key
                    , int &start_index, int &end_index, bool &end_search);


};

// class LogRecord{
// public:
//     char key_[KEY_LEN];
//     uint64_t time_ = 0;
//     int offset_ = 0;

//     LogRecord* next_ = nullptr;
//     LogRecord* prev_ = nullptr;

//     MemoryBulk* meory_bulk_ = nullptr;

//     LogRecord(){
//         memset(key_, 0, KEY_LEN);
//     }

//     LogRecord(const char* key, uint64_t time, MemoryBulk* meory_bulk, int offset) 
//             : time_(time)
//             , meory_bulk_(meory_bulk)
//             , offset_(offset)
//             {
//                 lgy_strncpy(key_, key, KEY_LEN - 1);
//             }

//     // please remember to delete[]
//     char* Get_content();
//     const char* Get_content_ref(int &len);
// };





#endif