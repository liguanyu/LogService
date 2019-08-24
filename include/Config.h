#ifndef CONFIG_H
#define CONFIG_H


#define PERFORMACE_PRINT 1



#define SERVER_PORT 5000

const int BUFFER_SIZE = 1024;
const int Q_LEN = 1024;
const int STR_LEN = 1024;
const char server_shm_pathname[] = "./log_server";

const int CLIENT_Q_LEN = 128;
const int CLIENT_STR_LEN = 1024; 
const char sender_shm_pathname[] = "./log_sender";

const int NETWORK_TIMEOUT = 6000;

const int epoll_wait_time = 0;

const char SERVER_ADDR[] = "127.0.0.1";


const int BULKSIZE = 4096 * 512;
// const int BULKSIZE = 4096 * 16;

// const int MAP_INIT_NUM = 1 << 20;
// const int MAP_INIT_NUM = 1 << 14;
const int MAP_INIT_NUM = BULKSIZE / (STR_LEN);

const char log_directory[] = "./logs";

const int MAX_RECENT_QUEUE_LEN = 128;

const int QueryResult_SIZE = 4096 * 4;
const int QueryResult_Q_LEN = 16;


const int SEARCH_KEYWORD_LEN = 32;
const int KEY_LEN = 16;

const int FILE_NAME_LEN = 32;


const int BULK_QUEUE_LEN = 64;
// const int BULK_QUEUE_LEN = 16;
const int PERSISTENCE_THRESHOLD = 48;
// const int PERSISTENCE_THRESHOLD = 8;
const int RELEASE_TIME = 12000;
const int AUTOSAVE_TIME = 30000;


const int PERSISTENCE_SLEEP_TIME = 100;


const int MAX_NUM_RESULT = 50000;
#endif