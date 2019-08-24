#ifndef LOG_SERVICE_H
#define LOG_SERVICE_H

#include <string>
int init_cody_log_service(const char* server_id, int shm_num);
int cody_log(const char *fmt, ...);
int cody_log_string(std::string &s);
int close_cody_log_service();

int debug_send_persistense();

#endif