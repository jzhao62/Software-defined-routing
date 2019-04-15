cmake_minimum_required (VERSION 2.6)

#SET(CMAKE_CXX_FLAGS -pthread)


project (p3)
include_directories(include)
include_directories(src)
add_executable(p3
        include/author.h
        include/connection_manager.h
        include/control_handler.h
        include/control_header_lib.h
        include/global.h
        include/network_util.h
        src/author.c
        src/connection_manager.c
        src/control_handler.c
        src/control_header_lib.c
        src/network_util.c
        src/swetankk_assignment3.c)

