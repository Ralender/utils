cmake_minimum_required(VERSION 3.10)
project(utils)

set(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS_INIT "-Wall -W -Wextra")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -DNDEBUG")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")

add_executable(run_test test/any_callable_ref_test.cpp
    test/any_callable_test.cpp)
target_include_directories(run_test PUBLIC ".")
target_link_libraries(run_test -lgtest -lgtest_main -lpthread)

