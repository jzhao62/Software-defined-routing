cmake_minimum_required (VERSION 2.6)

#SET(CMAKE_CXX_FLAGS -pthread)


project (p3)
include_directories(include)
include_directories(src)
add_executable(p3
        include/connection_manager.h
        include/control_handler.h
        include/data_handler.h
        include/global.h
        include/header.h
        include/utils.h
        src/bilinshi_assignment3.cpp
        src/connection_manager.cpp
        src/control_handler.cpp
        src/data_handler.cpp
        src/header.cpp
        src/utils.cpp)

