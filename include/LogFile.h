/* This header is for log file store on disk */

#ifndef LOGFILE_H
#define LOGFILE_H

#include <cstring>
#include <fstream>
#include "MyType.h"
#include "Config.h"

#define OPEN_NEW_LOG_FILE_MODE (std::ios::ate|std::ios::out|std::ios::trunc)
#define OPEN_EXISTING_LOG_FILE_MODE (std::ios::in)

class MemoryBulk;
class LogFile
{
public:
    int num_;
    char name_[FILE_NAME_LEN];
    std::fstream file_;

    LogFile(int num) : num_(num) {}

    void New_file();
    int Open_existing_file();
    int Close(){
        if(file_.is_open()){
            file_.close();
        }
    }

    ~LogFile()
    {
        Close();
    }

    // virtual void Save_data_to_file(MemoryBulk* bulk);

};


class LogContentFile : public LogFile
{
public:
    LogContentFile(int num) : LogFile(num){
        snprintf(name_, FILE_NAME_LEN, "%020d.log", num);
    }

    void Save_log_content_file(MemoryBulk* bulk);
    void Read_log_content_file(MemoryBulk* bulk);

    // remember to delete[]
    char* Get_content_by_offset(int offset, int len);

};

class LogIndexFile : public LogFile
{
public:
    LogIndexFile(int num) : LogFile(num){
        snprintf(name_, FILE_NAME_LEN, "%020d.index", num);
    }

    int Save_index_file(OffsetList &offset_list);
    int Read_index_file(OffsetList &offset_list);

};

class LogTimeStampFile : public LogFile
{
public:
    LogTimeStampFile(int num) : LogFile(num){
        snprintf(name_, FILE_NAME_LEN, "%020d.ts", num);
    }

    int Save_timestamp_file(TimeStampList &timestamp_list);
    int Read_timestamp_file(TimeStampList &timestamp_list);
    
};

class LogTrieFile : public LogFile
{
public:
    LogTrieFile(int num) : LogFile(num) {}

    // int Save_trie_file(SerialTrie &serial_trie);
    // int Read_trie_file(SerialTrie &serial_trie);
};

class LogKeyTrieFile : public LogTrieFile
{
public:
    LogKeyTrieFile(int num) : LogTrieFile(num){
        snprintf(name_, FILE_NAME_LEN, "%020d.keyt", num);
    }

    int Save_trie_file(KeyTrie &serial_trie);
    int Read_trie_file(KeyTrie &serial_trie);
};

class LogWordTrieFile : public LogTrieFile
{
public:
    LogWordTrieFile(int num) : LogTrieFile(num){
        snprintf(name_, FILE_NAME_LEN, "%020d.wordt", num);
    }

    int Save_trie_file(KeyWordTrie &serial_trie);
    int Read_trie_file(KeyWordTrie &serial_trie);

};


#endif