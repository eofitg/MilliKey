cmake_minimum_required(VERSION 3.15)
project(MilliKey C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

add_executable(MilliKey
        main.c
)

# 链接 macOS ApplicationServices 框架
target_link_libraries(MilliKey "-framework ApplicationServices")
