/* This componet is a library for user to link to send log*/


#include <cstdarg>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <cstdint>

#include "wrap.h"
#include "Epoll.h"
#include "MsgQueue.h"
#include "Log_service.h"
#include "UrlEncode.h"

#include "Config.h"

typedef char CLIENT_STR[CLIENT_STR_LEN];
typedef MyMsgQueue<CLIENT_Q_LEN, CLIENT_STR> MsgQueue;
int shm_id = 0;
MsgQueue *msg_queue;

static char server_id[KEY_LEN];
static uint64_t key = 3;
static uint64_t Get_time_sec(){
    key++;
    return key/3;
}

int start = 1;
int rand30()
{
    start = (start + 4159) % 48413;
    return start % 30;
}

char key_array[] = "vtxvccMh6FLkWqV7ZlnsRIwaZcURnFquFlTWvlKLOgv86LNzUobAj488ExgUDUKu4jvjl2RnpipITStA7ELMrf5Ytz";
std::string key_l[30];

const std::string& Get_key()
{
    return key_l[rand30()];
}

// static uint64_t Get_time_sec(){
//     struct timeval tv;
//     int ret = gettimeofday(&tv,NULL);
//     if(ret != 0){
//         err_printf("gettimeofday: ");
//     }
//     return tv.tv_sec;
// }

int init_cody_log_service(const char* server_id_, int shm_num)
{
    lgy_strncpy(server_id, server_id_, KEY_LEN - 1);
    void* shm_ptr = Get_shm(shm_id, sizeof(MsgQueue), sender_shm_pathname, shm_num);
    if(shm_ptr == nullptr){
        err_sys("shm_ptr is nullptr\n");
    }
    msg_queue = reinterpret_cast<MsgQueue* > (shm_ptr);
    while(!msg_queue->isInit());


    for(int i=0; i<30; i++){
        char a[10];
        strncpy(a, key_array+i*3, 10);
        key_l[i] = a;
    }
}


int close_cody_log_service()
{
    printf("end log service\n");
}


int cody_log(const char *fmt, ...)
{
    va_list		ap;

    CLIENT_STR msg;
    memset(msg, 0, CLIENT_STR_LEN);
	va_start(ap, fmt);
	vsnprintf(msg, CLIENT_STR_LEN - 1, fmt, ap);

    char* encode_str = url_encode(msg);
    char* key_str = url_encode(server_id);

    std::stringstream ss;
    ss  << "GET /append?"
        << "key=" << Get_key() << '&'
        << "time=" << Get_time_sec() << '&'
        << "content=" << encode_str << '&'
        << " HTTP/1.1";

    delete[] encode_str;
    delete[] key_str;

    lgy_strncpy(msg, ss.str().c_str(), CLIENT_STR_LEN - 2);

    int ret = 0;
    while(ret == 0){
        ret = msg_queue->WriteToMsgQueue_Wait(&msg, CLIENT_STR_LEN);
    }

	va_end(ap);
}



int cody_log_string(std::string &s)
{
    url_encode(s);

    std::stringstream ss;
    ss  << "GET /append?"
        << "key=" << Get_key() << '&'
        << "time=" << Get_time_sec() << '&'
        << "content=" << s << '&'
        << " HTTP/1.1";

    CLIENT_STR msg;
    memset(msg, 0, CLIENT_STR_LEN);
    lgy_strncpy(msg, ss.str().c_str(), CLIENT_STR_LEN - 2);

    int ret = 0;
    while(ret == 0){
        ret = msg_queue->WriteToMsgQueue(&msg, CLIENT_STR_LEN);
    }
}


int debug_send_persistense()
{
    CLIENT_STR str = "GET /persistence& HTTP/1.1\n";
    int ret = 0;
    while(ret == 0){
        ret = msg_queue->WriteToMsgQueue(&str, CLIENT_STR_LEN);
    }
}