/*This componet is to send log to server */

#include <iostream>
#include <cstring>
#include <string>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>         
#include <sys/resource.h>       // for 最大连接数需要setrlimit
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/tcp.h>

#include "wrap.h"
#include "Epoll.h"
#include "Config.h"
#include "MsgQueue.h"

typedef char CLIENT_STR[CLIENT_STR_LEN];
typedef MyMsgQueue<CLIENT_Q_LEN, CLIENT_STR> MsgQueue;
CLIENT_STR msg_send_buffer_instance;
CLIENT_STR *msg_send_buffer = &msg_send_buffer_instance;
MsgQueue *msg_queue;
struct sockaddr_in server_address;


int Init_msg_queue(int shm_num){
    int shm_id = 0;
    void* shm_ptr = Init_shm(shm_id, sizeof(MsgQueue), sender_shm_pathname, shm_num );
    msg_queue = new (shm_ptr) MsgQueue();
    return shm_id;
}

int Close_msg_queue(int shm_id){
    while(!msg_queue->Empty());
    sem_wait(&msg_queue->read_sem_);    
    Close_shm(shm_id);
}

int Connect_to_Server()
{
    int sockfd = socket( PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0 );
    if( sockfd < 0 ){
        printf( "socket failed\n" );
        return 1;
    }
    
    Connect( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) );
    return sockfd;
}

int Read_data_from_msgqueue()
{
    return msg_queue->ReadAndDeleteFromMsgQueue(msg_send_buffer);
}

int DoWithEpoll(Epoll_Obj& epoll_obj, int Ready_fd_number){
    char buf[ BUFFER_SIZE ];
    for ( int i = 0; i < Ready_fd_number; i++ )
    {
        int sockfd = epoll_obj.events_[i].data.fd;
        // printf("epoll got fd is %d\n", sockfd);

        if(epoll_obj.events_[i].events & (EPOLLOUT)){
            struct tcp_info info; 
            socklen_t info_len = sizeof(info);
            getsockopt(sockfd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t*)&info_len ); 
            if( info.tcpi_state != TCP_ESTABLISHED ){
                // err_printf("connect fail, %d\n", info.tcpi_state);
                epoll_obj.DelFd(sockfd);
                close(sockfd);
                continue;
            }

            // printf("here\n");

            // while(1){
                // int read_data = Read_data_from_msgqueue();
                int read_data;
                while( (read_data = Read_data_from_msgqueue()) == 0   );
                if(read_data != 0){

                // printf("%s\n", msg_send_buffer);
                    int data_send = 0;
                    int send_index = 0;
                    if(read_data != 0){
                        while( 1 )
                        {
                            data_send = send( sockfd, msg_send_buffer + send_index, read_data - send_index, 0 );
                            if( data_send < 0 )
                            {
                                if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
                                {
                                    // printf( "send later\n" );
                                    break;
                                    // continue;
                                }
                                printf( "sth wrong\n" );

                                break;
                            }
                            // else if( data_send == 0 )
                            // {
                            //     break;
                            // }

                            send_index += data_send;
                            if( send_index >= read_data){
                                break;
                            }
                        }
                    }
                }
            // }
            // epoll_obj.DelFd(sockfd);
            // close(sockfd);

            epoll_obj.SetRead(sockfd);

            // printf("send one msg\n");
        }
        
        if ( epoll_obj.events_[i].events & (EPOLLIN))
        {
            int recv_index = 0;
            memset( buf, '\0', BUFFER_SIZE );
            while( 1 )
            {
                int ret = recv( sockfd, buf + recv_index, BUFFER_SIZE-1 - recv_index, 0 );
                if( ret < 0 )
                {
                    if( ( errno == EAGAIN ) || ( errno == EWOULDBLOCK ) )
                    {
                        // printf( "read later\n" );
                        break;
                    }
                    break;
                }
                else if( ret == 0 )
                {
                    break;
                }
                else
                {
                    // printf( "get %d bytes of content: \n\n%s\n\n\n", ret, buf );
                    recv_index += ret;
                    if(recv_index >= BUFFER_SIZE){
                        break;
                    }
                }
            }
           
            if(recv_index > 0){
                // printf("%d, %d, %s\n", sockfd, recv_index, buf);
                // printf("del sockfd %d\n", sockfd);
                epoll_obj.DelFd(sockfd);
                close(sockfd);
                continue;
            }


            if (epoll_obj.events_[i].events & ( EPOLLRDHUP )){
                // printf("want to close fd %d\n", sockfd);

                epoll_obj.DelFd(sockfd);
                close(sockfd);

                // sockfd = Connect_to_Server();
                // connect( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) );
                // printf("re connect\n");
                // epoll_obj.AddFd(sockfd);
                // epoll_obj.SetRead(sockfd);

                break;
            }

            epoll_obj.SetWrite(sockfd);
        }

    }
}


int main( int argc, char* argv[] )
{
    if( argc <= 1 )
    {
        printf( "usage: %s shm_num\n", basename( argv[0] ) );
        return 1;
    }
    int shm_num = atoi( argv[1] );

    int port = SERVER_PORT;

    memset( &server_address, 0, sizeof( server_address ) );
    server_address.sin_family = AF_INET;
    inet_pton( AF_INET, SERVER_ADDR, &server_address.sin_addr );
    server_address.sin_port = htons( port );

    int sockfd = Connect_to_Server();

    // init shm
    int shm_id = Init_msg_queue(shm_num);

    Epoll_Obj epoll_obj;
    epoll_obj.AddFd(sockfd);
    epoll_obj.SetWrite(sockfd);

    while(1){
        int ret = epoll_obj.Epoll_Wait(0);
        if(ret == -1 ){
            break;
        }

        DoWithEpoll(epoll_obj, ret);

        while(epoll_obj.size_ < 100){
            sockfd = Connect_to_Server();
            connect( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) );
            // printf("re connect\n");
            // printf("new connect %d\n", sockfd);
            epoll_obj.AddFd(sockfd);
            // epoll_obj.SetRead(sockfd);

            epoll_obj.SetWrite(sockfd);
        }


    }
    
    close( sockfd );

    Close_msg_queue(shm_id);

    return 0;
}
