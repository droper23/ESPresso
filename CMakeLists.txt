cmake_minimum_required(VERSION 4.0)
project(ESPresso C)

set(CMAKE_C_STANDARD 11)

add_executable(ESPresso main.c
        lexer.h
        lexer.c)
