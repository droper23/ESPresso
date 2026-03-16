cmake_minimum_required(VERSION 4.0)
project(ESPresso C)

set(CMAKE_C_STANDARD 11)

add_executable(ESPresso
        src/main.c
        src/lexer.h
        src/lexer.c
        src/ast.h
        src/parser.h
        src/parser.c
        src/ast_utils.h
        src/ast_utils.c
        src/eval.c
        src/eval.h
        src/env.c
        src/env.h
        src/value.c
        src/value.h
)
#
#add_executable(test_env
#        test_env.c
#        env.c
#)
#
#add_executable(test_eval
#        test_eval.c
#        eval.c
#        env.c
#        ast_utils.c
#        parser.c
#        lexer.c
#)