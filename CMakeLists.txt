cmake_minimum_required(VERSION 3.19)
project(EasyWebServer)

set(CMAKE_CXX_STANDARD 11)

find_package(Boost)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    add_executable(EasyWebServer
            main_http.cpp
            server_base.hpp
            server_http.hpp
            handler.hpp)
endif()
