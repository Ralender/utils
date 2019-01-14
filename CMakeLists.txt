cmake_minimum_required(VERSION 3.12)
project(utils)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")

set(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS_INIT "-Wall -W -Wextra")

set(DEBUG_OPTIONS "-ggdb3")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast -march=native -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")

add_executable(test test/callable_ref_test.cpp)
target_include_directories(test PUBLIC ".")
target_link_libraries(test PUBLIC gtest gtest_main)

add_executable(utils main.cpp)
target_link_libraries(utils PUBLIC pthread)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_DEBUG_LIBS OFF)

find_package(Threads REQUIRED)
find_package(Boost 1.60.0 COMPONENTS signals system thread REQUIRED)
link_directories(${Boost_LIBRARY_DIR})
add_definitions(${Boost_LIB_DIAGNOSTIC_DEFINITIONS})

#set(CMAKE_CXX_FLAGS "-static")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lbenchmark -lbenchmark_main -lpthread")

add_executable(bench bench.cpp)
target_link_libraries(bench PUBLIC benchmark benchmark_main pthread)

#target_include_directories(utils PUBLIC conf src ${Boost_INCLUDE_DIRS})
#target_link_libraries(utils ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
