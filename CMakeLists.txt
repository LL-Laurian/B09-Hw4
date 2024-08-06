cmake_minimum_required(VERSION 3.22.1)
project(A4 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(A4
        client.c
        record.h
        server.c)
