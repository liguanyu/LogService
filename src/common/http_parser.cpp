/* to parse http req */

#include <iostream>
#include <cstdlib>
#include <climits>
#include <cstring>

#include "Http_parser.h"
#include "wrap.h"
#include "UrlEncode.h"

#include "Config.h"

const char method_str[] = "GET";
const char append_str[] = "append";
const char recent_str[] = "recent";
const char search_str[] = "search";
const char searchtime_str[] = "searchtime";
const char key_str[] = "key";
const char time_str[] = "time";
const char content_str[] = "content";
const char count_str[] = "count";
const char persistence_str[] = "persistence";
const char getkey_str[] = "getkey";
const char keyword_str[] = "keyword";
const char start_str[] = "start";
const char end_str[] = "end";
const char id_str[] = "id";
const char logcount_str[] = "logcount";


static int Find_char(const char* req_str, char c)
{
    int ret = 0;
    while(*req_str != c && *req_str != '\0' 
            && *req_str != ' ' && *req_str != '\n'){
        req_str++;
        ret++;
    } 
    if(*req_str == c){
        return ret;
    }else{
        return -1;
    }
}


enum class ParameterType{
    Invalid = 0,
    Key,
    Time,
    Content,
    Count,
    Keyword,
    Start,
    End,
    Id
};

bool Get_parameter(const char* str, ReqData* req_data)
{
    int index = Find_char(str, '=');
    if(index == -1){
        return false;
    }   

    // decide para type
    ParameterType para_type = ParameterType::Invalid;
    if(strncmp(str, keyword_str, strlen(keyword_str)) == 0){
        para_type = ParameterType::Keyword;
    }else if(strncmp(str, key_str, strlen(key_str)) == 0){
        para_type = ParameterType::Key;
    }else if(strncmp(str, time_str, strlen(time_str)) == 0){
        para_type = ParameterType::Time;
    }else if(strncmp(str, content_str, strlen(content_str)) == 0){
        para_type = ParameterType::Content;
    }else if(strncmp(str, count_str, strlen(count_str)) == 0){
        para_type = ParameterType::Count;
    }else if(strncmp(str, start_str, strlen(start_str)) == 0){
        para_type = ParameterType::Start;
    }else if(strncmp(str, end_str, strlen(end_str)) == 0){
        para_type = ParameterType::End;
    }else if(strncmp(str, id_str, strlen(id_str)) == 0){
        para_type = ParameterType::Id;
    }

    // get para value
    const char* parameter_start = str + index + 1;
    char* pEnd;
    switch(para_type){
        case ParameterType::Key:{
            char* req_data_str = new char[strlen(parameter_start) + 1];
            lgy_strncpy(req_data_str, parameter_start, strlen(parameter_start));
            char* str_decode = url_decode(req_data_str);
            delete[] req_data_str;
            lgy_strncpy(req_data->key_, str_decode, KEY_LEN - 1);
            delete[] str_decode;
            return true;
        }
        case ParameterType::Time:{
            uint64_t ret = strtol(parameter_start, &pEnd, 0);
            if(ret == 0L || ret == LONG_MAX || ret == LONG_MIN ){
                return false;
            }
            req_data->time_ = ret;
            return true;
        }
        case ParameterType::Start:{
            uint64_t ret = strtol(parameter_start, &pEnd, 0);
            if(ret == 0L || ret == LONG_MAX || ret == LONG_MIN ){
                return false;
            }
            req_data->start_ = ret;
            return true;
        }
        case ParameterType::End:{
            uint64_t ret = strtol(parameter_start, &pEnd, 0);
            if(ret == 0L || ret == LONG_MAX || ret == LONG_MIN ){
                return false;
            }
            req_data->end_ = ret;
            return true;
        }
        case ParameterType::Id:{
            uint64_t ret = strtol(parameter_start, &pEnd, 0);
            if(ret == 0L || ret == LONG_MAX || ret == LONG_MIN ){
                return false;
            }
            req_data->client_id_ = ret;
            return true;
        }
        case ParameterType::Content:{
            char* req_data_str = new char[strlen(parameter_start) + 1];
            lgy_strncpy(req_data_str, parameter_start, strlen(parameter_start));
            char* str_decode = url_decode(req_data_str);
            delete[] req_data_str;
            lgy_strncpy(req_data->content_, str_decode, STR_LEN - 1);
            delete[] str_decode;
            return true;
        }
        case ParameterType::Count:{
            uint64_t ret = strtol(parameter_start, &pEnd, 0);
            if(ret == 0L || ret == LONG_MAX || ret == LONG_MIN ){
                return false;
            }
            req_data->count_ = ret;
            return true;
        }case ParameterType::Keyword:{
            char* req_data_str = new char[strlen(parameter_start) + 1];
            lgy_strncpy(req_data_str, parameter_start, strlen(parameter_start));
            char* str_decode = url_decode(req_data_str);
            delete[] req_data_str;
            lgy_strncpy(req_data->keyword_, str_decode, SEARCH_KEYWORD_LEN - 1);
            delete[] str_decode;
            return true;
        }
        default:{
            break;
        }
    }

    return false;
    
}

ReqData* ParseReq(char* req_str)
{
    int read_index = 0;
    ReqType type;

    // find "GET"
    if(strncmp(req_str + read_index, method_str, strlen(method_str)) != 0){
        return nullptr;
    }
    read_index += strlen(method_str) + 1;

    // decide type
    if(*(req_str + read_index) != '/'){
        return nullptr;
    }
    read_index += sizeof('/');

    if(strncmp(req_str + read_index, append_str, strlen(append_str)) == 0){
        type = ReqType::Append;
        read_index += strlen(append_str);
    }else if(strncmp(req_str + read_index, recent_str, strlen(recent_str)) == 0){
        type = ReqType::Recent;
        read_index += strlen(recent_str);
    }else if(strncmp(req_str + read_index, getkey_str, strlen(getkey_str)) == 0){
        type = ReqType::Getkey;
        read_index += strlen(recent_str);
    }else if(strncmp(req_str + read_index, persistence_str, strlen(persistence_str)) == 0){
        type = ReqType::Persistence;
        read_index += strlen(persistence_str);
    }else if(strncmp(req_str + read_index, searchtime_str, strlen(searchtime_str)) == 0){
        type = ReqType::SearchTime;
        read_index += strlen(searchtime_str);
    }else if(strncmp(req_str + read_index, search_str, strlen(search_str)) == 0){
        type = ReqType::Search;
        read_index += strlen(search_str);
    }else if(strncmp(req_str + read_index, logcount_str, strlen(logcount_str)) == 0){
        type = ReqType::LogCount;
        read_index += strlen(logcount_str);
    }else{
        return nullptr;
    }

    // get parameters 
    switch(type){
        case ReqType::Append:
        case ReqType::Recent:
        case ReqType::Getkey:
        case ReqType::Search:
        case ReqType::SearchTime:
        case ReqType::LogCount:
        case ReqType::Persistence:
        {
            ReqData* req_data = new ReqData();

            int separator_index = Find_char(req_str + read_index, '?');
            if(separator_index != -1){
                // find space
                int space_index = Find_char(req_str + read_index + separator_index + 1, ' ');
                if(space_index == -1){
                    delete req_data;
                    return nullptr;
                }
                *(req_str + read_index + separator_index + 1 + space_index) = '\0';

                // ananlyse parameter
                while(separator_index != -1){
                    read_index += separator_index + 1;
                    while( *(req_str + read_index) == '&' ){
                        read_index++;
                    }
                    separator_index = Find_char(req_str + read_index, '&');
                    if(separator_index != -1){
                        *(req_str + read_index + separator_index) = '\0';
                    }

                    Get_parameter(req_str + read_index, req_data);

                }
            }

            req_data->type_ = type;
            return req_data;
        }
        default:{
            return nullptr;
        }
    }

    return nullptr;
}