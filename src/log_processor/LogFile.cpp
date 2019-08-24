#include <string>
#include <queue>
#include <typeinfo>

#include "LogFile.h"
#include "Data_manager.h"


void LogFile::New_file()
{
    if(access(log_directory, F_OK) != 0)
    {
        if(mkdir(log_directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {
            err_sys("cannot mkdir %s\n", log_directory);
        }
    }
    std::string filename = log_directory;
    filename += '/';
    filename += name_;

    file_.open(filename, OPEN_NEW_LOG_FILE_MODE);
    if(!file_.is_open()){
        err_sys("LogFile open failed, name_ %s\n", name_);
    }
}

int LogFile::Open_existing_file()
{
    if(access(log_directory, F_OK) != 0)
    {
        if(mkdir(log_directory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
        {
            err_sys("cannot mkdir %s\n", log_directory);
        }
    }
    std::string filename = log_directory;
    filename += '/';
    filename += name_;

    file_.open(filename, OPEN_EXISTING_LOG_FILE_MODE);
    if(!file_.is_open()){
        err_sys("LogFile open failed, name_ %s\n", name_);
    }
}

void LogContentFile::Save_log_content_file(MemoryBulk* bulk)
{
    if(!file_.is_open()){
        New_file();
    }

    file_ << bulk->data_->buffer_;
    
    Close();
}

void LogContentFile::Read_log_content_file(MemoryBulk* bulk)
{
    if(!file_.is_open()){
        Open_existing_file();
    }

    file_.seekg(0,std::ios::beg);
    file_.read(bulk->data_->buffer_, sizeof(bulk->data_->buffer_) );


    Close();
}

char* LogContentFile::Get_content_by_offset(int offset, int len)
{
    char* buffer = new char[len + 1];
    file_.seekg(offset,std::ios::beg);
    file_.read(buffer, len);
    buffer[len] = '\0';

    return buffer;
}

struct SerializationUnitKey{
    TrieNode<int, false> * node_;
    int father_ = 0;
 
    SerializationUnitKey(TrieNode<int, false> * node, int father)
            : node_(node), father_(father) {}
};
int LogKeyTrieFile::Save_trie_file(KeyTrie &serial_trie)
{

    New_file();
    file_ << serial_trie.size_ << std::endl;

    std::queue<SerializationUnitKey> q;
    q.push(SerializationUnitKey(serial_trie.root_, -1));
    int count = 0;
    while(!q.empty())
    {
        auto unit = q.front();
        q.pop();

        file_ << unit.father_ << ' '
                << unit.node_->c_ << ' '
                << unit.node_->value_list_.size() << ' ';
        
        for(int &offset : unit.node_->value_list_){
            file_ << offset << ' ';
        }
        file_ << std::endl;
        
        for(auto &pair : unit.node_->char_map_){
            q.push(SerializationUnitKey(pair.second, count) );
        }

        count++;
    }

    serial_trie.Delete_tree();
    serial_trie.SetReadOnly();
    Close();

    return count;
}

int LogKeyTrieFile::Read_trie_file(KeyTrie &serial_trie)
{
    Open_existing_file();
    int node_num;
    file_ >> node_num;

    TrieNode<int, false>* node_array = (TrieNode<int, false>*) malloc(sizeof( TrieNode<int, false>[node_num] ) );

    // the root node
    int father;
    int value_list_len;
    char c;
    file_ >> father
            >> c
            >> value_list_len;
    if(father != -1){
        err_sys("read trie fail");
    }
    TrieNode<int, false>* start = new(node_array) TrieNode<int, false>(c);
    if(value_list_len != 0){
        err_sys("read trie fail");
    }
    
    for(int i = 1; i < node_num; i++)
    {
        file_ >> father
                >> c
                >> value_list_len;
        TrieNode<int, false>* node = new(node_array+i) TrieNode<int, false>(c);

        node_array[father].char_map_.insert(
                std::make_pair(c, &node_array[i]) );

        node_array[i].c_ = c;

        std::vector<int> value_list(value_list_len, 0);
        for(int j = 0; j < value_list_len; j++){
            file_ >> value_list[j];
        }
        node_array[i].value_list_.swap(value_list);
    }

    serial_trie.root_ = &node_array[0];
    serial_trie.Set_valid();
    serial_trie.size_ = node_num;
    serial_trie.store_in_array = true;

    Close();
    return node_num;
}


struct SerializationUnitKeyword{
    TrieNode<int, true> * node_;
    int father_ = 0;
 
    SerializationUnitKeyword(TrieNode<int, true> * node, int father)
            : node_(node), father_(father) {}
};
int LogWordTrieFile::Save_trie_file(KeyWordTrie &serial_trie)
{

    New_file();
    file_ << serial_trie.size_ << std::endl;

    std::queue<SerializationUnitKeyword> q;
    q.push(SerializationUnitKeyword(serial_trie.root_, -1));
    int count = 0;
    while(!q.empty())
    {
        auto unit = q.front();
        q.pop();

        file_ << unit.father_ << ' '
                << unit.node_->c_ << ' '
                << unit.node_->value_list_.size() << ' ';
        
        for(int &offset : unit.node_->value_list_){
            file_ << offset << ' ';
        }
        file_ << std::endl;
        
        for(auto &pair : unit.node_->char_map_){
            q.push(SerializationUnitKeyword(pair.second, count) );
        }

        count++;
    }

    serial_trie.Delete_tree();
    serial_trie.SetReadOnly();
    Close();

    return count;
}

int LogWordTrieFile::Read_trie_file(KeyWordTrie &serial_trie)
{
    Open_existing_file();
    int node_num;
    file_ >> node_num;

    TrieNode<int, true>* node_array = (TrieNode<int, true>*) malloc(sizeof( TrieNode<int, true>[node_num] ) );

    // the root node
    int father;
    int value_list_len;
    char c;
    file_ >> father
            >> c
            >> value_list_len;
    if(father != -1){
        err_sys("read trie fail");
    }
    TrieNode<int, true>* start = new(node_array) TrieNode<int, true>(c);
    if(value_list_len != 0){
        err_sys("read trie fail");
    }
    
    for(int i = 1; i < node_num; i++)
    {
        file_ >> father
                >> c
                >> value_list_len;
        TrieNode<int, true>* node = new(node_array+i) TrieNode<int, true>(c);

        node_array[father].char_map_.insert(
                std::make_pair(c, &node_array[i]) );

        node_array[i].c_ = c;

        std::vector<int> value_list(value_list_len, 0);
        for(int j = 0; j < value_list_len; j++){
            file_ >> value_list[j];
        }
        node_array[i].value_list_.swap(value_list);
    }

    serial_trie.root_ = &node_array[0];
    serial_trie.Set_valid();
    serial_trie.size_ = node_num;
    serial_trie.store_in_array = true;

    Close();
    return node_num;
}

int LogIndexFile::Save_index_file(OffsetList &offset_list)
{

    New_file();
    int len = offset_list.size();
    file_ << len << std::endl;

    for(int i = 0; i < len; i++){
        file_ << i << " " << offset_list[i] << std::endl;
    }

    //delete list
    OffsetList tmp;
    offset_list.swap(tmp);

    Close();
    return len;
}


int LogIndexFile::Read_index_file(OffsetList &offset_list)
{
    Open_existing_file();
    int len = 0;
    file_ >> len;
    OffsetList tmp(len, 0);

    int index; //useless
    for(int i = 0; i < len; i++){
        file_ >> index >> tmp[i];
    }

    offset_list.swap(tmp);
    Close();
    return len;
}


int LogTimeStampFile::Save_timestamp_file(TimeStampList &timestamp_list)
{

    New_file();
    int len = timestamp_list.size();
    file_ << len << std::endl;

    for(int i = 0; i < len; i++){
        file_ << i << " " << timestamp_list[i] << std::endl;
    }

    //delete list
    TimeStampList tmp;
    timestamp_list.swap(tmp);

    Close();
    return len;
}


int LogTimeStampFile::Read_timestamp_file(TimeStampList &timestamp_list)
{

    Open_existing_file();
    int len = 0;
    file_ >> len;
    TimeStampList tmp(len, 0);

    int index; //useless
    for(int i = 0; i < len; i++){
        file_ >> index >> tmp[i];
    }

    timestamp_list.swap(tmp);
    Close();
    return len;
}


