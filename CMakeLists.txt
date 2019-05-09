cmake_minimum_required(VERSION 2.8) 
FIND_PACKAGE(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 11)
INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS})    

#################boost#################
find_package(Boost 1.54.0 COMPONENTS filesystem system program_options) 

include_directories(${Boost_INCLUDE_DIRS})

add_executable(ntripclient ./main.cpp ./ntripclient.h ./ntripclient.cpp)    

target_link_libraries(ntripclient pthread ${OpenCV_LIBS} ${Boost_LIBRARIES})

