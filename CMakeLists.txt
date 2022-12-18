cmake_minimum_required(VERSION 3.20.5)

project(proxys)

set(CMAKE_CXX_STANDARD 17)

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.80.0 COMPONENTS system thread)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    add_executable(proxy main.cpp)
    target_link_libraries(proxy ${Boost_LIBRARIES})
endif()