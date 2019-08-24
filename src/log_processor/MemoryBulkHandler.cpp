#include "Data_manager.h"
#include "LogFile.h"
#include "Persistence_worker.h"

void MemoryBulkHandler::New_BulkData_init()
{
    BulkData* data = nullptr;
    while( data == nullptr ){
        data = bulk_data_queue.Get_Unit();
    }

    bulk_->data_ = data;
    data->bulk_handler_ = this;

    if(state_ == MemoryBulkState::Init){
        state_ = MemoryBulkState::Appending;
    }
}

void MemoryBulkHandler::New_BulkData()
{
    BulkData* data = new BulkData();

    bulk_->data_ = data;
    data->bulk_handler_ = this;

}

int MemoryBulkHandler::Save_Content()
{
    state_ = MemoryBulkState::Saved;
    content_file_.Save_log_content_file(bulk_);
}

int MemoryBulkHandler::Read_Content()
{
    if(bulk_->data_ == nullptr){
        New_BulkData();
    }
    content_file_.Read_log_content_file(bulk_);
    state_ = MemoryBulkState::InMemory;
}

const char* MemoryBulkHandler::Get_data()
{
    if(state_ == MemoryBulkState::Saved){
        Read_Content();
        state_ = MemoryBulkState::InMemory;
        Set_timer();
    }
    if(bulk_ == nullptr){
        err_sys("Get_data error\n");
    }

    return reinterpret_cast<const char*> (bulk_->data_->buffer_);
}


int MemoryBulkHandler::Save_index()
{
    if(index_state_.offset_read_ == false){
        return 1;
    }
    index_state_.offset_read_ = false;
    index_file_.Save_index_file(Get_index()->offset_list_);
}

int MemoryBulkHandler::Read_index()
{
    index_file_.Read_index_file(Get_index()->offset_list_);
    index_state_.offset_read_ = true;
}

int MemoryBulkHandler::Save_timestamp()
{
    if(index_state_.timestamp_read_ == false){
        return 1;
    }
    index_state_.timestamp_read_ = false;
    timestamp_file_.Save_timestamp_file(Get_index()->timestamp_list_);
}

int MemoryBulkHandler::Read_timestamp()
{
    timestamp_file_.Read_timestamp_file(Get_index()->timestamp_list_);
    index_state_.timestamp_read_ = true;
}

int MemoryBulkHandler::Save_key_trie()
{
    if(index_state_.key_read_ == false){
        return 1;
    }  
    index_state_.key_read_ = false;
    key_file_.Save_trie_file(Get_index()->key_serial_trie_);
}

int MemoryBulkHandler::Read_key_trie()
{
    key_file_.Read_trie_file(Get_index()->key_serial_trie_);
    index_state_.key_read_ = true;
}

int MemoryBulkHandler::Save_word_trie()
{    
    if(index_state_.word_read_ == false){
        return 1;
    }
    index_state_.word_read_ = false;
    word_file_.Save_trie_file(Get_index()->keyword_serial_trie_);
}

int MemoryBulkHandler::Read_word_trie()
{   
    word_file_.Read_trie_file(Get_index()->keyword_serial_trie_);
    index_state_.word_read_ = true;
}


int MemoryBulkHandler::Delete_Content()
{
    if(state_ == MemoryBulkState::Appending){
        bulk_->data_ = nullptr;
    }
    if(state_ != MemoryBulkState::Saved && bulk_->data_ != nullptr){
        delete bulk_->data_;
        bulk_->data_ = nullptr;
    }

    state_ = MemoryBulkState::Saved;
}

int MemoryBulkHandler::Delete_index()
{
    if(index_state_.offset_read_ == true){
        OffsetList tmp;
        bulk_->index_.offset_list_.swap(tmp);
    }
    index_state_.offset_read_ = false;
}

int MemoryBulkHandler::Delete_timestamp()
{
    if(index_state_.timestamp_read_ == true){
        TimeStampList tmp;
        bulk_->index_.timestamp_list_.swap(tmp);
    }
    index_state_.timestamp_read_ = false;
}

int MemoryBulkHandler::Delete_key_trie()
{
    if(index_state_.key_read_ == true){
        bulk_->index_.key_serial_trie_.Delete_tree();
    }
    index_state_.key_read_ = false;
}

int MemoryBulkHandler::Delete_word_trie()
{
    if(index_state_.word_read_ == true){
        bulk_->index_.keyword_serial_trie_.Delete_tree();
    }
    index_state_.word_read_ = false;
}



int MemoryBulkHandler::Persistence()
{
    if(state_ != MemoryBulkState::InMemory 
            && state_ != MemoryBulkState::InQueue
            && state_ != MemoryBulkState::Appending){
        return -1;
    }
    Get_write_lock();
    state_ = MemoryBulkState::Saved;

    Save_Content();

    Save_index();
    Save_timestamp();
    Save_key_trie();
    Save_word_trie();

    // TODO
    if(bulk_->data_ != nullptr){
        bulk_->data_->bulk_handler_ = nullptr;
    }
    bulk_->data_ = nullptr;

    Drop_write_lock();
    return 1;
    //TODO
}



int MemoryBulkHandler::Get_offset_by_serial(int serial)
{
    if(index_state_.offset_read_ == false){
        Read_index();
    }
    int ret = bulk_->Get_offset_by_serial(serial);
    return ret;
}

OffsetList* MemoryBulkHandler::Search_key(const char* key)
{
    if(index_state_.key_read_ == false){
        Read_key_trie();
    }
    // if(state_ == MemoryBulkState::Saved){
    //     Read_Content();
    // }
    Set_timer();
    return bulk_->Search_key(key);
}


OffsetList* MemoryBulkHandler::Search_keyword(const char* keyword)
{
    if(index_state_.word_read_ == false){
        Read_word_trie();
    }
    // if(state_ == MemoryBulkState::Saved){
    //     Read_Content();
    // }
    Set_timer();
    return bulk_->Search_keyword(keyword);
}

OffsetList MemoryBulkHandler::Join_search(std::vector<std::string> &word_list)
{
    if(index_state_.word_read_ == false){
        Read_word_trie();
    }
    // if(state_ == MemoryBulkState::Saved){
    //     Read_Content();
    // }
    Set_timer();
    return bulk_->Join_search(word_list);
}



OffsetList* MemoryBulkHandler::Search_time_key(uint64_t start, uint64_t end, const char* key
            , int &start_index, int &end_index, bool &end_search)
{
    if(start > end_time_ ){
        end_search = true;
        return nullptr;
    }

    if(end < start_time_){
        return nullptr;
    }

    if(index_state_.key_read_ == false){
        Read_key_trie();
    }
    if(index_state_.timestamp_read_ == false){
        Read_timestamp();
    }
    Set_timer();
    return bulk_->Search_time_key(start, end, key, start_index, end_index, end_search);
}
