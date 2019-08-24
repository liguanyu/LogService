PROJECT(cody_log_service)

add_compile_options(-std=c++11 -g)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/release)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/release)

ADD_SUBDIRECTORY(./src)
