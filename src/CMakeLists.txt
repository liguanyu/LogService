SET(INC ${PROJECT_SOURCE_DIR}/include)
include_directories(${INC})

ADD_SUBDIRECTORY(./log_server)
ADD_SUBDIRECTORY(./log_sender)
ADD_SUBDIRECTORY(./log_service)


SET(URL_ENCODE_SOURCE ./common/url_encode.cpp)
SET(HTTP_PARSER_SOURCE ./common/http_parser.cpp)

SET(LOG_SERVER_SOURCE ./log_server/log_server.cpp ${HTTP_PARSER_SOURCE} ${URL_ENCODE_SOURCE})
SET(LOG_SENDER_SOURCE ./log_sender/log_sender.cpp)
SET(SHORT_SENDER_SOURCE ./log_sender/short_sender.cpp)
SET(LOG_SERVICE_SOURCE ./log_service/log_service.cpp ${URL_ENCODE_SOURCE})
SET(TEST_SOURCE ./test_log_producer/main.cpp)
SET(LOG_PROCESSOR_SOURCE ./log_processor/log_processor.cpp ${HTTP_PARSER_SOURCE} ./log_processor/data_manager.cpp ${URL_ENCODE_SOURCE} 
                    ./log_processor/LogFile.cpp ./log_processor/MemoryBulkHandler.cpp
                    ./log_processor/Persistence_worker.cpp)


ADD_EXECUTABLE(log_server ${LOG_SERVER_SOURCE})
ADD_EXECUTABLE(log_sender ${LOG_SENDER_SOURCE})
ADD_EXECUTABLE(short_sender ${SHORT_SENDER_SOURCE})
ADD_EXECUTABLE(test_log_producer ${TEST_SOURCE})
ADD_EXECUTABLE(log_processor ${LOG_PROCESSOR_SOURCE})

add_library(log_service ${LOG_SERVICE_SOURCE})

TARGET_LINK_LIBRARIES(log_server pthread)
TARGET_LINK_LIBRARIES(log_sender pthread)
TARGET_LINK_LIBRARIES(short_sender pthread)
TARGET_LINK_LIBRARIES(log_service pthread)
TARGET_LINK_LIBRARIES(test_log_producer log_service)
TARGET_LINK_LIBRARIES(log_processor pthread)
