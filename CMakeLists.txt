cmake_minimum_required(VERSION 3.25)
project(Project_Fuzzing C)

set(CMAKE_C_STANDARD 11)

add_executable(Project_Fuzzing
        src/fuzzer.c)
