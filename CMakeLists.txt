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
        src/chunk.c
        src/chunk.h
        src/compiler.c
        src/compiler.h
        src/vm.c
        src/vm.h
        src/debug.c
        src/debug.h
)

enable_testing()
add_test(NAME asan_program
        COMMAND ${CMAKE_SOURCE_DIR}/tests/run_asan.sh
        $<TARGET_FILE:ESPresso>
        ${CMAKE_SOURCE_DIR}/examples/program.espr
)
add_test(NAME valgrind_program
        COMMAND ${CMAKE_SOURCE_DIR}/tests/run_valgrind.sh
        $<TARGET_FILE:ESPresso>
        ${CMAKE_SOURCE_DIR}/examples/program.espr
)
add_test(NAME interp_suite
        COMMAND ${CMAKE_SOURCE_DIR}/tests/run_interp_suite.sh
        $<TARGET_FILE:ESPresso>
        ${CMAKE_SOURCE_DIR}/examples/program.espr
)
add_test(NAME parse_errors
        COMMAND ${CMAKE_SOURCE_DIR}/tests/run_parse_errors.sh
        $<TARGET_FILE:ESPresso>
        ${CMAKE_SOURCE_DIR}/examples/parse_errors/unclosed_string.espr
        ${CMAKE_SOURCE_DIR}/examples/parse_errors/missing_brace.espr
        ${CMAKE_SOURCE_DIR}/examples/parse_errors/bad_for.espr
)
add_test(NAME vm_smoke
        COMMAND ${CMAKE_SOURCE_DIR}/tests/run_vm_suite.sh
        $<TARGET_FILE:ESPresso>
        ${CMAKE_SOURCE_DIR}/examples/program.espr
)
add_test(NAME trace_disasm
        COMMAND ${CMAKE_SOURCE_DIR}/tests/run_trace_disasm.sh
        $<TARGET_FILE:ESPresso>
        ${CMAKE_SOURCE_DIR}/examples/program.espr
)
if(APPLE)
    add_test(NAME leaks_program
            COMMAND ${CMAKE_SOURCE_DIR}/tests/run_leaks.sh
            $<TARGET_FILE:ESPresso>
            ${CMAKE_SOURCE_DIR}/examples/program.espr
    )
endif()
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
