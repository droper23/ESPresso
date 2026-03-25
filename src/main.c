// main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lexer.h"
#include "parser.h"
#include "eval.h"
#include "env.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

Value nativePrint(int argCount, Value* args) {
    for (int i = 0; i < argCount; i++) {
        printValue(args[i]);
        if (i < argCount - 1) printf(" ");
    }
    printf("\n");
    return makeNull();
}

static double nowSeconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static Env* createGlobalEnv(void) {
    Env* global = create_environment(NULL);
    env_define(global, "print", makeNative(nativePrint));
    env_define(global, "log", makeNative(nativePrint));
    return global;
}

static int runInterpreterOnce(ASTNode* program, const char* source, const char* filename) {
    Env* global = createGlobalEnv();
    evalSetSource(source, filename);
    ASTNode* stmt = program;
    while (stmt) {
        Value result = evaluate(stmt, global);
        if (result.type == VALUE_RETURN) {
            freeValueContents(result);
            break;
        }
        freeValueContents(result);
        stmt = stmt->next;
    }
    free_environment_force(global);
    return 0;
}

static InterpretResult runVMOnce(Chunk* chunk, const char* source, const char* filename, int trace, int disasm) {
    Env* global = createGlobalEnv();
    evalSetSource(source, filename);
    vmSetSource(source, filename);
    vmSetTrace(trace);
    if (disasm) {
        disassembleChunk(chunk, "script");
    }
    InterpretResult result = interpret(chunk, global);
    vmFree();
    free_environment_force(global);
    return result;
}

int main(int argc, char* argv[]) {
    int useInterp = 0;
    int trace = 0;
    int disasm = 0;
    int bench = 0;
    int runs = 20;
    const char* filename = NULL;

    if (argc < 2) {
        printf("Usage: %s [--interp] [--trace] [--disasm] [--bench] [--runs N] <file.espr>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--interp") == 0) {
            useInterp = 1;
        } else if (strcmp(argv[i], "--trace") == 0) {
            trace = 1;
        } else if (strcmp(argv[i], "--disasm") == 0) {
            disasm = 1;
        } else if (strcmp(argv[i], "--bench") == 0) {
            bench = 1;
        } else if (strcmp(argv[i], "--runs") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --runs requires a value.\n");
                return 1;
            }
            runs = atoi(argv[++i]);
            if (runs <= 0) {
                fprintf(stderr, "Error: --runs must be > 0.\n");
                return 1;
            }
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        printf("Usage: %s [--interp] [--trace] [--disasm] [--bench] [--runs N] <file.espr>\n", argv[0]);
        return 1;
    }

    // Read file
    FILE* f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = '\0';
    fclose(f);

    // Initialize lexer and parser
    Lexer lexer;
    initLexer(&lexer, source);

    Parser parser;
    initParser(&parser, &lexer);

    // Parse statements into AST
    ASTNode* program = NULL;
    ASTNode* last = NULL;
    while (parser.current.type != TOKEN_EOF) {
        ASTNode* stmt = parseStatement(&parser);
        if (!program) {
            program = stmt;
        } else {
            last->next = stmt;
        }
        last = stmt;
    }
    freeParser(&parser);

    if (bench) {
        Chunk chunk;
        initChunk(&chunk);
        if (!compile(program, &chunk)) {
            freeChunk(&chunk);
            freeAST(program);
            free(source);
            return 1;
        }

        double t0 = nowSeconds();
        for (int i = 0; i < runs; i++) {
            runInterpreterOnce(program, source, filename);
        }
        double t1 = nowSeconds();

        double t2 = nowSeconds();
        for (int i = 0; i < runs; i++) {
            runVMOnce(&chunk, source, filename, 0, 0);
        }
        double t3 = nowSeconds();

        double interp = t1 - t0;
        double vm = t3 - t2;
        printf("Interpreter (%d runs): %.6f seconds\n", runs, interp);
        printf("VM (%d runs): %.6f seconds\n", runs, vm);
        if (vm > 0.0) {
            printf("Speedup: %.2fx\n", interp / vm);
        } else {
            printf("Speedup: infx\n");
        }

        freeChunk(&chunk);
    } else if (useInterp) {
        runInterpreterOnce(program, source, filename);
    } else {
        Chunk chunk;
        initChunk(&chunk);
        if (!compile(program, &chunk)) {
            freeChunk(&chunk);
            freeAST(program);
            free(source);
            return 1;
        }
        InterpretResult result = runVMOnce(&chunk, source, filename, trace, disasm);
        freeChunk(&chunk);
        if (result != INTERPRET_OK) {
            freeAST(program);
            free(source);
            return 1;
        }
    }

    // Cleanup
    freeAST(program);
    free(source);

    return 0;
}
