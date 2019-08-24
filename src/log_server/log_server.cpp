/* This component is to recieve req and pass it to log_processor */

#include <iostream>
#include <cstring>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>         
// #include <fcntl.h>              
#include <sys/resource.h>      
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unordered_map>
#include <vector>
#include <sys/time.h>
#include <sstream>

#include "wrap.h"
#include "TimerWheel.h"
#include "Epoll.h"
#include "MsgQueue.h"
#include "Http_parser.h"
#include "QueryResult.h"

#include "Config.h"

typedef MyMsgQueue<Q_LEN, ReqData> MsgQueue;
typedef MyMsgQueue<QueryResult_Q_LEN, QueryResult> ResultQueue;


char msg_rscv_buffer[ BUFFER_SIZE ];
MsgQueue *msg_queue;
ResultQueue *result_queue;

int Init_msg_queue(){
    int shm_id = 0;
    void* shm_ptr = Init_shm(shm_id, sizeof(MsgQueue) + sizeof(ResultQueue), server_shm_pathname );
    msg_queue = new (shm_ptr) MsgQueue();
    result_queue = new (reinterpret_cast<void *> (msg_queue + 1) ) ResultQueue();
    return shm_id;
}

int Close_msg_queue(int shm_id){
    // while(!msg_queue->Empty());
    sem_wait(&msg_queue->read_sem_);    
    sem_wait(&result_queue->read_sem_);    
    Close_shm(shm_id);
}


std::unordered_map<int, int> client_id_fd_map;
std::unordered_map<int, int> fd_client_id_map;
int Insert_client_id_fd_map(int client_id, int fd)
{
    client_id_fd_map.emplace(client_id, fd);
    fd_client_id_map.emplace(fd, client_id);
}

int Find_client_id_fd_map(int client_id)
{
    auto iter = client_id_fd_map.find(client_id);
    if(iter == client_id_fd_map.end()){
        return 0;
    }
    
    return iter->second;
}

int Delete_client_id_fd_map(int fd)
{
    auto iter = fd_client_id_map.find(fd);
    if(iter == fd_client_id_map.end()){
        return 0;
    }

    int client_id = iter->second;
    client_id_fd_map.erase(client_id);
    fd_client_id_map.erase(fd);
}

class FdTimeout:public TimerBase
{
public:
    int fd_;
    Epoll_Obj* epoll_obj_; 
    void Act(){
        epoll_obj_->DelFd(fd_);
        close(fd_);
    }

    static std::unordered_map<int, FdTimeout*> map_;

    FdTimeout(int fd, Epoll_Obj* epool_obj)
        : fd_(fd), epoll_obj_(epool_obj) 
    {
        map_.insert(std::make_pair(fd_, this));
    }


    static FdTimeout* GetFdTimeout(int fd)
    {
        return map_[fd];
    }

    ~FdTimeout(){
        map_.erase(fd_);
    }

};
std::unordered_map<int, FdTimeout*> FdTimeout::map_;


std::set<int> fd_ack_set;

static uint64_t Get_time_microsec(){
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
}

#if (PERFORMACE_PRINT == 1)

uint64_t a = 0;
uint64_t b = 0;
uint64_t unit = 10000;
uint64_t show_qps(){
    b = Get_time_microsec();
    // std::cout << (b - a) << std::endl;
    uint64_t tmp = b-a;
    uint64_t qps = unit * 1000000 / tmp;
    std::cout << qps << std::endl;
    a = Get_time_microsec();
    return qps;
}
uint64_t req_counts = 0;
uint64_t add_qps = 0;
uint64_t qps_count = 0;
#endif

bool Write_msg_to_msgqueue(int fd)
{

#if (PERFORMACE_PRINT == 1)
    req_counts++;
    if(req_counts % unit == 0 && req_counts > 20*unit){
        uint64_t qps = show_qps();
        if(qps > 10){
            add_qps += qps;
            qps_count ++;
            std::cout << "aver. " << add_qps/qps_count << std::endl;

        }
    }
#endif

    bool need_send_back = false;
    int index = 0;
    for(; index < BUFFER_SIZE/STR_LEN; index++){
        if(*(msg_rscv_buffer + index*STR_LEN) == 0 ){
            break;
        }else{
            // char *tmp_str = new char[strlen(msg_rscv_buffer + index*STR_LEN)+1];
            // lgy_strncpy(tmp_str, msg_rscv_buffer + index*STR_LEN, strlen(msg_rscv_buffer + index*STR_LEN));

            // ReqData* req_data = ParseReq(tmp_str);
            // if(req_data == nullptr || !req_data->Valid()){
            //     std::cout<< msg_rscv_buffer + index*STR_LEN << std::endl;
            // }

            ReqData* req_data = ParseReq(msg_rscv_buffer + index*STR_LEN);
            if(req_data != nullptr){
                // printf("send %s\n", req_data->content_);
                int client_id = req_data->Need_send_back();
                if(client_id > 0){
                    Insert_client_id_fd_map(client_id, fd);
                    need_send_back = true;
                }
                int ret = msg_queue->WriteToMsgQueue_Wait(req_data);
                if(ret == 0){
                    printf("queue full\n");
                }
                delete req_data;
            }

            // delete[] tmp_str;
        }
    }

    return need_send_back;
}


void Del_fd_form_fd_result_map(int fd);
std::unordered_map<int, std::vector<QueryResult* > > fd_result_map;

void Del_fd_from_result_index_strucure(int fd)
{
    Del_fd_form_fd_result_map(fd);
    Delete_client_id_fd_map(fd);
}


void Read_fd_work(int sockfd, Epoll_Obj &epoll_obj)
{
    int done = 0;
    memset( msg_rscv_buffer, '\0', BUFFER_SIZE );
    while(done == 0){
        int data_read = 0;
        int read_index = 0;
        int checked_index = 0;
        int start_line = 0;
        while( 1 )
        {
            data_read = recv( sockfd, msg_rscv_buffer + read_index, BUFFER_SIZE - read_index, 0 );
            if ( data_read == -1 )
            {
            if(errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    done = 1;
                    break;
                }
                // printf("close connection\n");
                epoll_obj.DelFd(sockfd);
                Del_fd_from_result_index_strucure(sockfd);
                close(sockfd);
                done = 1;
                break;
            }
            else if ( data_read == 0 )
            {
                // if(errno == EAGAIN){
                //     done = 1;
                // }
                // printf( "remote client has closed the connection\n" );
                // printf("close connection\n");
                epoll_obj.DelFd(sockfd);
                Del_fd_from_result_index_strucure(sockfd);
                close(sockfd);
                done = 1;
                break;
            }

            read_index += data_read;
            if(read_index >= BUFFER_SIZE){
                break;
            }
        }

        // if(read_index >= BUFFER_SIZE){
        //     done = 1;
        // }

        // printf("rscv: %d %s\n",read_index,  msg_rscv_buffer);

        if(read_index != 0){
            bool need_send_back = Write_msg_to_msgqueue(sockfd);
            if(!need_send_back){
                epoll_obj.SetWrite(sockfd);
                fd_ack_set.insert(sockfd);
            }
        }
    }

    // printf("want to close connection fd %d\n", sockfd);
    // epoll_obj.DelFd(sockfd);
    // timer_wheel.DelTimer(FdTimeout::GetFdTimeout(sockfd));
    // FdTimeout* fd_timeout = new FdTimeout(sockfd, &epoll_obj);
    // timer_wheel.AddTimer(fd_timeout, NETWORK_TIMEOUT);

    // printf("close fd return %d\n", close(sockfd));

    // if (epoll_obj.events_[i].events & ( EPOLLRDHUP )){
    //     printf("close connection\n");
    //     epoll_obj.DelFd(sockfd);
    //     Del_fd_form_fd_result_map(sockfd);
    //     close(sockfd);
    // }

}

void MySend(int sockfd, const char* msg, int len)
{
    int data_send = 0;
    int send_index = 0;
    while( 1 )
    {
        data_send = send( sockfd, msg + send_index, len - send_index, 0 );
        // printf("data_send is %d, len is %d\n", data_send, len);
        if( data_send < 0 )
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
            {
                // printf( "send later\n" );
                continue;
            }
            break;
        }


        send_index += data_send;
        if( send_index >= len){
            break;
        }
    }
}

const char ack[] = "[ACK]\n";

void Ack_to_fd(int sockfd, Epoll_Obj &epoll_obj)
{
    auto iter_sendback = fd_ack_set.find(sockfd);
    if(iter_sendback != fd_ack_set.end()){

        std::stringstream ss;
        ss << "HTTP/1.0 200 OK\r\n"
            << "Content-length: " << strlen(ack) << "\r\n"
            << "Content-type: text/plain\r\n\r\n";
        MySend(sockfd, ss.str().c_str(), ss.str().size() );
        MySend(sockfd, ack, strlen(ack));
        fd_ack_set.erase(sockfd);
        epoll_obj.SetRead(sockfd);
        // Del_fd_from_result_index_strucure(sockfd);
        // epoll_obj.DelFd(sockfd);
        // close(sockfd);
    }
}

void Result_to_fd(int sockfd, Epoll_Obj &epoll_obj)
{
    auto iter = fd_result_map.find(sockfd);
    if(iter == fd_result_map.end()){
        return;
    }

    std::vector<QueryResult*> &query_result_list = iter->second;
    int finish = 0;
    int msg_len = 0;
    for(QueryResult* &query_result : query_result_list){
        msg_len += query_result->offset_;
    }

    std::stringstream ss;
    ss << "HTTP/1.0 200 OK\r\n"
        << "Content-length: " << msg_len << "\r\n"
        << "Content-type: text/plain\r\n\r\n";

    MySend(sockfd, ss.str().c_str(), ss.str().size() );

    QueryResult* time_block = query_result_list.back();
    query_result_list.pop_back();
    auto it = query_result_list.begin();
    query_result_list.insert(it, time_block);

    for(QueryResult* &query_result : query_result_list){
            // printf("%s\n", msg_send_buffer);
        // printf("%s\n", query_result->Get_Logs() );
        int len = query_result->offset_;
        MySend(sockfd, query_result->Get_Logs(), len);
        finish = query_result->finish_;
        // delete query_result;
    }
    Del_fd_from_result_index_strucure(sockfd);
    if(finish == 1)
    {                
        printf("close connection\n");
        epoll_obj.DelFd(sockfd);
        close(sockfd);
    }
}

void Write_fd_work(int sockfd, Epoll_Obj &epoll_obj)
{
    // printf("want to send, %d\n", sockfd);

    Ack_to_fd(sockfd, epoll_obj);

    Result_to_fd(sockfd, epoll_obj);


}


void DoWithEpoll(Epoll_Obj& epoll_obj, int Ready_fd_number, TimerWheelSet<FdTimeout> &timer_wheel){
    for ( int i = 0; i < Ready_fd_number; i++ )
    {
        int sockfd = epoll_obj.events_[i].data.fd;
        // printf("epoll got fd is %d\n", sockfd);

        if ( sockfd == epoll_obj.listen_fd_ )
        {
            while(true){
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof( client_address );
                int connfd = accept4( epoll_obj.listen_fd_, ( struct sockaddr* )&client_address, &client_addrlength, SOCK_NONBLOCK );
                if(connfd == -1){
                    if(errno == EAGAIN){
                        break;
                    }else{
                        err_sys("accept error : %d\n", errno);
                    }
                }
                epoll_obj.AddFd(connfd);
            }

            // FdTimeout* fd_timeout = new FdTimeout(connfd, &epoll_obj);
            // timer_wheel.AddTimer(fd_timeout, NETWORK_TIMEOUT);

        }
        else
        {
            // recv
            if ( epoll_obj.events_[i].events & EPOLLIN )
            {
                Read_fd_work(sockfd, epoll_obj);
            }
            // send back
            if(epoll_obj.events_[i].events & EPOLLOUT){
                Write_fd_work(sockfd, epoll_obj);
            }
            // else
            // {
            //     printf( "something else happened \n" ); 
            // }

        }

    }
}


// void Writer_thread();

void Del_fd_form_fd_result_map(int fd)
{
    auto iter = fd_result_map.find(fd);
    if(iter != fd_result_map.end()){
        for(QueryResult* result : iter->second){
            delete result;
        }
        fd_result_map.erase(fd);
    }
}

void Insert_fd_qurey_result(int fd, QueryResult* result)
{
    auto iter = fd_result_map.find(fd);
    if(iter == fd_result_map.end()){
        std::vector<QueryResult* > tmp;
        fd_result_map.insert(std::make_pair(fd, tmp));
        iter = fd_result_map.find(fd);
    }

    iter->second.push_back(result);
}

const int MAX_QUERY_RESULT_READ = QueryResult_Q_LEN/2;

int Read_result_queue(std::vector<int> &fd_list)
{
    int read = 0;
    while(read <= MAX_QUERY_RESULT_READ ){
        QueryResult* result = new QueryResult(0);
        int ret = result_queue->ReadAndDeleteFromMsgQueue(result);
        if(ret == 0){
            delete result;
            return read;
        }
        // printf("%d, %s\n", result->client_id_, result->Get_Logs() );
        int fd = Find_client_id_fd_map(result->client_id_);
        if(fd != 0){
            Insert_fd_qurey_result(fd, result);
            if(result->Is_finish()){
                fd_list.push_back(fd);
            }
        }
        
        read++;
    }

    return read;
}

int main( int argc, char* argv[] )
{
    int port = SERVER_PORT;
    const char* ip;
    struct sockaddr_in address, client_address;
    int listen_fd, fd;
    int epoll_fd;
    int ret;
    socklen_t client_addrlength;

    // if( argc <= 1 )
    // {
    //     printf( "usage: %s port_number\n", basename( argv[0] ) );
    //     return 1;
    // }
    // port = atoi( argv[1] );

    printf("port is %d\n", port);

    
    memset( &address, 0, sizeof( address ) );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons( port );
    
    if((listen_fd = socket( PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0 )) < 0){
        err_sys("socket error : %d ...\n", errno);
    }

    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    printf("listen_fd is %d\n", listen_fd);

    if(bind( listen_fd, ( struct sockaddr* )&address, sizeof( address ) ) == -1 ){
        err_sys("bind error : %d ...\n", errno);
    }
    
    if(listen( listen_fd, 5 ) < 0){
        err_sys("listen error : %d ...\n", errno);
    }

    // init msg queue
    int shm_id = Init_msg_queue();
    
    Epoll_Obj epoll_obj;
    epoll_obj.AddListenFd(listen_fd);

    TimerWheelSet<FdTimeout> timer_wheel;


    while(1){
        std::vector<int> fd_list;

        int read = Read_result_queue(fd_list);

        if(read > 0){
            for(int &fd : fd_list){
                // printf("set write %d\n", fd);
                epoll_obj.SetWrite(fd);
            }
        }

        int ret = epoll_obj.Epoll_Wait();
        DoWithEpoll(epoll_obj, ret, timer_wheel);
        // timer_wheel.Tick();
    }
    
    close( listen_fd );

    Close_msg_queue(shm_id);

    return 0;
}




// void Writer_thread()
// {
//     Epoll_Obj epoll_obj;

//     while(1){
//         vector<int> fd_list;
//         int read = Read_result_queue(fd_list);

//         if(read > 0){
//             for(int &fd : fd_list){
//                 if(!epoll_obj.IsWaited(fd) ){
//                     epoll_obj.Add(fd, WRITE_MODE);
//                 }
//             }
//         }

//         int ret = epoll_obj.Epoll_Wait();
//         DoEpollWriteThread();

//     }
// }