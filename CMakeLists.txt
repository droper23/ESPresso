cmake_minimum_required(VERSION 4.0)
project(ESPresso C)

set(CMAKE_C_STANDARD 11)

add_executable(ESPresso main.c
        lexer.h
        lexer.c
        ast.h
        parser.h
        parser.c
        ast_utils.h
        ast_utils.c
        eval.c
        eval.h
        env.c
        env.h
        test_env.c
)

add_executable(test_env
        test_env.c
        env.c
)