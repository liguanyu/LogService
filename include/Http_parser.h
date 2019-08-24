/* this file for process http req to get mothed and content */

#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "Config.h"
#include <cstdint>
#include <iostream>
#include <cstring>

enum class ReqType{
    Invalid = 0,
    Append,
    Recent,
    Getkey,
    Search,
    SearchTime,
    LogCount,
    Persistence    // for debug
};

template <typename Enumeration>
auto as_integer(Enumeration const value)
    -> typename std::underlying_type<Enumeration>::type
{
    return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}

class ReqData{
public:

    ReqType type_  = ReqType::Invalid;

    // for append and getkey
    char key_[KEY_LEN];

    // for append
    uint64_t time_ = 0;

    // for append
    char content_[STR_LEN];

    // for search
    char keyword_[SEARCH_KEYWORD_LEN];

    // for sendback
    int client_id_ = 0;

    // for recent
    uint64_t count_ = 0;

    // for searchtime
    uint64_t start_ = 0;
    uint64_t end_ = 0;

    ReqData(){
        memset(content_, 0, STR_LEN);
        memset(keyword_, 0, SEARCH_KEYWORD_LEN);
        memset(key_, 0, KEY_LEN);
    }

    ~ReqData(){
    }

    bool Valid(){
        switch(type_){
            case ReqType::Append:{
                return *key_ != 0 && time_ != 0;
            }
            case ReqType::Recent:{
                return count_ != 0 && client_id_ > 0;
            }
            case ReqType::Getkey:{
                return *key_ != 0 && client_id_ > 0;
            }
            case ReqType::Search:{
                return *keyword_ != '\0' && client_id_ > 0;
            }
            case ReqType::SearchTime:{
                return *key_ != '\0' && start_ != 0 
                            && end_ != 0 && client_id_ > 0;
            }
            case ReqType::Persistence:{
                return true && client_id_ > 0;
            }
            case ReqType::LogCount:{
                return true && client_id_ > 0;
            }
            case ReqType::Invalid:
            default:{
                return false;
            }
        }
        return false;
    }

    void Print()
    {
        // if(!Valid()){
        //     std::cout << "invalid req\n" << std::endl;
        //     return;
        // }

        std::cout << "Req:" << std::endl
                    << "Type: " << as_integer(type_) << std::endl
                    << "key: " << key_ << std::endl
                    << "time: " << time_ << std::endl
                    << "content: " << content_ << std::endl
                    << "keyword: " << keyword_ << std::endl
                    << "count: " << count_ << std::endl << std::endl;
    }

    void Set_client_id(int client_id) {client_id_ = client_id;}

    int Need_send_back(){
        switch(type_){
            case ReqType::Recent:
            case ReqType::Search:
            case ReqType::SearchTime:
            case ReqType::Getkey:
            case ReqType::LogCount:
            case ReqType::Persistence:{
                return client_id_;
            }
            default:{
                return 0;
            }
        }
        return 0;
    }

};

ReqData* ParseReq(char* req);


#endif