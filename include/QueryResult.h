/* This component defines the query result class data manager returns to log server */


#ifndef QUERYRESULT_H
#define QUERYRESULT_H

#include <cstring>

class QueryResult
{
private:
    /* data */
public:
    int client_id_;
    int offset_ = 0;
    bool begin_ = 0;
    bool finish_ = 0;

    char logs_[QueryResult_SIZE 
            - sizeof(client_id_) - sizeof(offset_) - sizeof(finish_)];

    int Append(const char* str, int len)
    {
        if(offset_ + len + 1>= sizeof(logs_)){
            return 0;
        }

        lgy_strncpy(logs_ + offset_, str, len);
        // printf("append : %s\n", logs_ + offset_);
        offset_ += len;
        return len;
    }

    const char* Get_Logs() {return logs_;}

    void Set_begin() {begin_ = 1;}
    void Set_finish() {finish_ = 1;}
    bool Is_begin() {return begin_;}
    bool Is_finish() {return finish_;}

    QueryResult(int client_id)
            : client_id_(client_id)
    {
        memset(logs_, 0, sizeof(logs_));
    }
    ~QueryResult() {}
};





#endif