cmake_minimum_required(VERSION 3.19)
project(myshell C)

set(CMAKE_C_STANDARD 11)

set(SRCS main.c vector.c parse.c run.c cmd.c error.c)

add_executable(myshell ${SRCS})

target_compile_definitions(myshell PRIVATE _GNU_SOURCE)
target_compile_options(myshell PRIVATE -Wall -Wextra -Wpedantic)
